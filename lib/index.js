"use strict";

const addon = require('bindings')('node-libnftables');

// Dictionary with the available flags to configure libnftables output
const NFT_FLAGS = {
    OUTPUT_DEFAULT: addon.LibNftables.OUTPUT_DEFAULT, // use the libnftables default output settings
    OUTPUT_ECHO: addon.LibNftables.OUTPUT_ECHO, // echo the commands sent to nftables, as well as the responses
    OUTPUT_HANDLE: addon.LibNftables.OUTPUT_HANDLE, // include ruleset handles. You will need these to delete rules
    OUTPUT_REVERSE_DNS: addon.LibNftables.OUTPUT_REVERSE_DNS, // translate IP addresses to names via reverse DNS lookup
    OUTPUT_SERVICE_NAME: addon.LibNftables.OUTPUT_SERVICE_NAME, // translate ports to service names as defined in /etc/services
    OUTPUT_STATELESS: addon.LibNftables.OUTPUT_STATELESS, // omit stateful information e.g. rule counters
    OUTPUT_JSON: addon.LibNftables.OUTPUT_JSON, // format output in JSON. See "man libnftables-json" for schema description
    OUTPUT_GUID: addon.LibNftables.OUTPUT_GUID, // translate numeric UID/GID to names as defined in /etc/passwd and /etc/group
    OUTPUT_NUMERIC_PROTOCOL: addon.LibNftables.OUTPUT_NUMERIC_PROTOCOL, // display layer 4 protocol numerically
    OUTPUT_NUMERIC_PRIORITY: addon.LibNftables.OUTPUT_NUMERIC_PRIORITY, // display base chain priority numerically
    OUTPUT_NUMERIC_SYMBOL: addon.LibNftables.OUTPUT_NUMERIC_SYMBOL, // display symbolic constants numerically
    OUTPUT_NUMERIC_TIME: addon.LibNftables.OUTPUT_NUMERIC_TIME, // show time values numerically
    OUTPUT_NUMERIC_ALL: addon.LibNftables.OUTPUT_NUMERIC_ALL, // fully numeric output
    OUTPUT_TERSE: addon.LibNftables.OUTPUT_TERSE // omit contents of sets and maps
}
Object.freeze(NFT_FLAGS);


class LibNftablesContext extends addon.LibNftables {
    /**
     * Create a libnftables context
     */
    constructor() {
        super();
        this._lastResponse = '';
        this._initContext();
    }

    /**
     * Refresh the libnftables context. You may need this to make sure ruleset counters are updated.
     * Releases the current libnftables context and creates a new context with the same output flags.
     * Not needed unless your counters are not updating often enough.
     * @returns {LibNftablesContext}
     * @throws {Error} unable to create the new context
     */
    refreshContext = () => {
        this._initContext();
        return this;
    }

    /**
     * Run nftables command(s), similar to using "nft -i".
     * See `man nftables` and `https://wiki.nftables.org/wiki-nftables/index.php/Main_Page` for examples.
     * Multiple command lines can be sent, delimited by the newline character (\n)
     * Returns the response string from nftables
     * @param nftCommand {string}- single or multiline string, as used by the nft command line utility
     * @returns {LibNftablesContext}
     * @throws {Error} nftables rejected a command
     */
    runCmd = (nftCommand) => {
        this._lastResponse = this._runCmd(nftCommand);
        return this;
    }

    /**
     * Returns the last nftables response as a string, including any tabs (\t) and newline (\n) characters
     * @returns {string}
     */
    asString = () => {
        return this._lastResponse;
    }

    /**
     * Returns the last nftables response as an array of response lines, filtered to omit tabs, braces and empty lines
     * @returns {string[]}
     */
    asArray = () => {
        const tabExp = /[\t{}]/gu;  // Matches tabs or braces
        const newLine = /\n+/u;  // one or more newlines (unix style)
        let ruleset = this._lastResponse.replace(tabExp, '').trim().split(newLine);
        return ruleset.filter(Boolean);  // omit empty lines
    }

    /**
     * Returns the last nftables response as a Map object with nftables handles for each rule.
     * You can use the nftables handle to delete a rule.
     * Note: Requires that the OUTPUT_HANDLE flag has been set, else the map will be empty
     * @returns Map { handle: rule }
     */
    asMap = () => {
        const handleExp = /\s+#\s+handle\s+/u;
        let ruleset = this.asArray();
        let handles = new Map();
        for (let rule of ruleset) {
            if (rule.match(/handle/)) {
                let pieces = rule.split(handleExp);
                if (pieces.length > 1) {
                    handles.set(pieces[1], pieces[0]);
                }
            }
        }
        return handles;
    }

    /**
     * Parses the last response from nftables as JSON and returns the response as an object.
     * The OUTPUT_JSON flag must have been set or JSON parsing will fail.
     * See libnftables-json(5) for a description of the supported schema.
     * @returns {any}
     * @throws {SyntaxError} libnftables did not return valid JSON
     */
    asObject = () => {
            return JSON.parse(this._lastResponse);
    }

    /**
     * Set output flags for the libnftables context. Overwrites any previous flag settings.
     * See NFTABLES_FLAGS for the valid flag values.
     * Flag values persist for the lifetime of the context unless overwritten by a new call to setOutputFlags().
     * @param {...int} flags - one or more output flags to set. All other flags will be cleared.
     * @returns {LibNftablesContext}
     * @throws {Error} invalid flag values
     */
    setOutputFlags = (...flags) => {
        this._setOutputFlags(...flags);
        return this;
    }

    /**
     * Set Dry Run Mode. When Dry Run is enabled libnftables will parse commands, but will not update the rule set.
     * @param dry {boolean}  - default True
     * @returns {LibNftablesContext}
     */
    setDryRun = (dry= true) => {
        this._dryRun = this._setDryRun(dry);
        return this;
    }
}

module.exports = { LibNftablesContext, NFT_FLAGS };
