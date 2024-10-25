"use strict";

const { LibNftablesContext, NFT_FLAGS } = require("../lib");
const assert = require("assert");

assert(LibNftablesContext, "The expected module is undefined");

const nftables = new LibNftablesContext();
assert(nftables, "Unable to create a libnftables context");

function testRunCmd () {
    nftables.runCmd('list ruleset');
}
assert.doesNotThrow(testRunCmd, undefined, 'runCmd threw an exception');


console.log("Tests passed");
