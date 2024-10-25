# linux-nftables
A native node add-on to view and modify nftables firewall rulesets on Linux.
For example, to build a paywall or a Wi-Fi hotspot. 

If you only want to deploy static nftables firewall rules then this is probably not the tool you need. 
E.g. on Debian just add the ruleset to /etc/nftables.conf, and Debian will re-install the rules on boot.

Refer to the Nftables Wiki for more information on nftables: 

https://wiki.nftables.org/wiki-nftables/index.php/Main_Page

## Security
To view or modify nftables firewall rules you must run node.js as a user account with CAP_NET_ADMIN capabilities.
So as well as being able to view and modify firewall configurations, the user account will also have sufficient
privileges to view and modify network interface configurations and routing tables. Be careful!

For example, confine your application within an unprivileged LXC container, and configure the global route table to
route traffic of interest via the LXC's virtual network interfaces. 

## Dependencies
- Linux kernel >= 4.14
- libnftables  >= 0.9.6
- Linux cmake, build-essential, and libnftables-dev packages

## Installing
Installation uses cmake-js to compile the add-on and link to libnftables on your platform.
Binaries are not included in the NPM module.

First, make sure you have a working build system including the libftables headers:
E.g. for Debian 12:
```
sudo apt install build-essential cmake curl libnftables-dev
```

Now use npm to install node.js dependencies and compile the add-on for your system:
```
npm install
```

## Testing
Loads the module and attempts to read the running nftables ruleset. Just a simple test to validate that the addon has
linked to libnftables and node is running as a user with sufficient privileges to manage nftables rulesets. 
```
sudo npm test
```

## Examples
```js
const { LibNftablesContext, NFT_FLAGS } = require('linux-libnftables');
const nftContext = new LibNftablesContext();

let baseRuleSet = [
    'add table ip paywall',    
    'add chain ip paywall forward { type filter hook forward priority 0; policy drop; }',
    'add set ip paywall authorised { type ipv4_addr; flags interval; }',
    'add rule ip paywall forward ip saddr @authorised accept', 
    'add rule ip paywall forward ip daddr @authorised ct state established,related accept',
    'add element ip paywall authorised { 10.1.2.3, 10.1.2.4, 192.168.1.0/24 }'
];

for (const rule of baseRuleSet) {
    nftContext.runCmd(rule);
}

nftContext.runCmd('list ruleset').asArray();  // returns an array of [rules]
>>> [
    'table ip paywall  # handle 32',
    'set authorised  # handle 2',
    'type ipv4_addr',
    'flags interval',
    'elements =  10.1.2.3, 10.1.2.4,',
    '     192.168.1.0/24 ',
    'chain forward  # handle 1',
    'type filter hook forward priority filter; policy drop;',
    'ip saddr @authorised accept # handle 3',
    'ip daddr @authorised ct state established,related accept # handle 4'
]


nftContext.setOutputFlags(NFT_FLAGS.OUTPUT_HANDLE); // include ruleset handles in the output
nftContext.runCmd('list ruleset').asMap();  // returns a Map of { handle: rule }
>>> Map(5) {
    '32' => 'table ip paywall',
        '2' => 'set authorised',
        '1' => 'chain forward',
        '3' => 'ip saddr @authorised accept',
        '4' => 'ip daddr @authorised ct state established,related accept'
}


nftContext.setOutputFlags(NFT_FLAGS.OUTPUT_JSON, NFT_FLAGS.OUTPUT_HANDLE); // JSON format, including ruleset handles
let rs = nftContext.runCmd('list ruleset').asObject();
rs
>>> {
    nftables: [
        { metainfo: [Object] },
        { table: [Object] },
        { set: [Object] },
        { chain: [Object] },
        { rule: [Object] },
        { rule: [Object] },
        { rule: [Object] },
        { rule: [Object] }
    ]
}
rs.nftables[0]
>>> {
    metainfo: {
        version: '1.0.6',
            release_name: 'Lester Gooch #5',
            json_schema_version: 1
    }
}


```

## Class LibNftablesContext

One libnftables context should be sufficient for most use cases. You may encounter  NF_NETLINK resource issues if you
try to use multiple libnftables contexts, or if you create and release libnftables contexts too rapidly.

All methods throw an Error if the command is not accepted by libnftables. The Error type and message may contain further information.

### Constructor
Creates a new libnftables context

### runCmd (nftCommand)
Run nftables command(s), similar to using "nft -i".

See `man nftables` and `https://wiki.nftables.org/wiki-nftables/index.php/Main_Page` for examples.

Multiple command lines can be sent, delimited by the newline character (\n)

@param nftCommand {string}- single or multiline string, as used by the nft command line utility

@returns {LibNftablesContext}

@throws {Error} nftables rejected a command

### asString ()
Returns the last nftables response as a string, including any tabs (\t) and newline (\n) characters

@returns {string}

### asArray ()
Returns the last nftables response as an array of response lines, filtered to omit tabs, braces and empty lines

@returns {string[]}

### asMap ()
Returns the last nftables response as a Map object with nftables handles for each rule.
You can use the nftables handle to delete a rule.

Note: Requires that the OUTPUT_HANDLE flag has been set, else the map will be empty

@returns Map { handle: rule }

### asObject ()
Parses the last response from nftables as JSON and returns the response as an object.
The OUTPUT_JSON flag must have been set or JSON parsing will fail.

See libnftables-json(5) for a description of the supported schema.

@returns {any}

@throws {SyntaxError} libnftables did not return valid JSON


### setOutputFlags (flag1, flag2, ...)
Set output flags for the libnftables context. See NFT_FLAGS for the valid flag values.
Overwrites any previous flag settings.

@param {...int} flags - one or more output flags to set. All other flags will be cleared.

@returns {LibNftablesContext}

@throws {Error} invalid flag values

```js
const { LibNftablesContext, NFT_FLAGS } = require('linux-libnftables');
const nftContext = new LibNftablesContext();

// Output ruleset handles, numeric output, omit contents of sets. All other flags are cleared.
nftContext.setOutputFlags(NFT_FLAGS.OUTPUT_HANDLE, NFT_FLAGS.OUTPUT_NUMERIC_ALL, NFT_FLAGS.OUTPUT_TERSE);
```

### dryRun (nftCommands)
Set Dry Run Mode. When Dry Run is enabled libnftables will parse commands, but will not update the rule set.

@param dry {boolean}  - default True

@returns {LibNftablesContext}

### refreshContext ()
Refresh the libnftables context. You may need this to make sure ruleset counters have been updated.
Releases the current libnftables context and creates a new context with the same output flags.
Not needed unless your counters are not updating frequently enough.

@returns {LibNftablesContext}

@throws {Error} unable to create the new context


## NFT_FLAGS
A helper object that provides the nftables output control flags for the installed version of libnftables

### OUTPUT_DEFAULT
Restore the default output settings for the installed version of libnftables

### OUTPUT_ECHO 
Echo the commands sent to nftables, as well as the responses.

### OUTPUT_HANDLE
Upon insertion into the ruleset, some elements are assigned a unique handle for identification purposes. For example, when deleting a table or chain, it may be identified either by name or handle. Rules on the other hand must be deleted by handle, because there is no other way to uniquely identify them. This flag makes ruleset listings include handle values

### OUTPUT_REVERSE_DNS
libnftables will use Reverse DNS lookups for IP addresses in the output. Note that this may add significant delay to commands, depending on the speed of your DNS resolver.

### OUTPUT_SERVICE_NAME
Translate ports to service names as defined in /etc/services

### OUTPUT_STATELESS
Omit stateful data e.g. packet and byte counters from the output.

### OUTPUT_JSON
Format output as JSON. See libnftables-json(5) for a description of the supported schema.
If enabled at compile-time, libnftables accepts commands in JSON format and is able to print output in JSON format as well.
This flag enables JSON output; the input command format is auto-detected.

### OUTPUT_GUID
Translate numeric UID/GID to names as defined in /etc/passwd and /etc/group

### OUTPUT_NUMERIC_PROTOCOL
Output layer 4 protocols numerically

### OUTPUT_NUMERIC_PRIORITY
Output nftables base chain priorities numerically

### OUTPUT_NUMERIC_SYMBOL
Output symbolic constants numerically

### OUTPUT_NUMERIC_TIME
Output time values numerically

### OUTPUT_NUMERIC_ALL
Sets OUTPUT_NUMERIC_PROTOCOL, OUTPUT_NUMERIC_PRIORITY, OUTPUT_NUMERIC_SYMBOL, and OUTPUT_NUMERIC_TIME

### OUTPUT_TERSE
Omit contents of nftables sets and maps.
