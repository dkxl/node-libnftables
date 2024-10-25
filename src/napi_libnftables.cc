#include <napi.h>
#include <nftables/libnftables.h>
#include "napi_libnftables.h"

// Constructor
LibNftables::LibNftables(const Napi::CallbackInfo& info) : ObjectWrap<LibNftables>(info) {
    this->ctx_ = nullptr;
}

// Release libnftables resources before destruction
LibNftables::~LibNftables() {
    if (this->ctx_ != nullptr) {
        nft_ctx_free(this->ctx_);
    }
}

// Releases any existing libnftables context and creates a new one. Output flags are restored.
void LibNftables::initContext(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    unsigned int curFlags = 0;

    if (this->ctx_ != nullptr) {
        curFlags = nft_ctx_output_get_flags(this->ctx_);
        nft_ctx_free(this->ctx_);
    }
    this->ctx_ = nft_ctx_new(NFT_CTX_DEFAULT);

    // Restore any previous non-default output flags
    if (!curFlags == 0) {
        nft_ctx_output_set_flags(this->ctx_, curFlags);
    }
    if (nft_ctx_buffer_output(this->ctx_) != 0) {
        Napi::Error::New(env, "output buffer could not be enabled").ThrowAsJavaScriptException();
    }
    if (nft_ctx_buffer_error(this->ctx_) != 0) {
        Napi::Error::New(env, "error buffer could not be enabled").ThrowAsJavaScriptException();
    }
}

// Send libnftables a command line string and return contents of the output buffer
Napi::Value LibNftables::runCmd(const Napi::CallbackInfo& info) {
    const Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expecting a single string argument").ThrowAsJavaScriptException();
        return env.Null();
    }
    const std::string cmd = info[0].As<Napi::String>().Utf8Value();
    char *cmd_buffer = strdup(cmd.c_str());
    const int rs = nft_run_cmd_from_buffer(this->ctx_, cmd_buffer);
    free(cmd_buffer);
    if (rs != 0) {
        Napi::Error::New(env, nft_ctx_get_error_buffer(this->ctx_)).ThrowAsJavaScriptException();
        return env.Null();
    }
    return Napi::String::New(env, nft_ctx_get_output_buffer(this->ctx_));
}

// When Dry Run is set libnftables will parse commands but will not update the ruleset
Napi::Value LibNftables::setDryRun(const Napi::CallbackInfo &info) {
    const Napi::Env env = info.Env();
    if (info.Length() == 0 || !info[0].IsBoolean() ) {
        Napi::TypeError::New(env, "Boolean argument required").ThrowAsJavaScriptException();
        return env.Null();
    }
    const bool requested = info[0].As<Napi::Boolean>().Value();
    nft_ctx_set_dry_run(this->ctx_, requested);
    const bool actual = nft_ctx_get_dry_run(this->ctx_);
    if (actual != requested) {
        Napi::Error::New(env, "Dry Run mode change failed").ThrowAsJavaScriptException();
        return env.Null();
    }
    return Napi::Boolean::New(env, actual);
}

// Get current state of Dry Run mode
Napi::Value LibNftables::getDryRun(const Napi::CallbackInfo &info) {
    const Napi::Env env = info.Env();
    return Napi::Boolean::New(env, nft_ctx_get_dry_run(this->ctx_));
}

// Get output flags for the libnftables context
Napi::Value LibNftables::getOutputFlags(const Napi::CallbackInfo &info) {
    const Napi::Env env = info.Env();
    return Napi::Number::New(env, nft_ctx_output_get_flags(this->ctx_));
}

// Set output flags for the libnftables context. Overwrites any previous flags
Napi::Value LibNftables::setOutputFlags(const Napi::CallbackInfo &info) {
    const Napi::Env env = info.Env();
    unsigned int flags = 0;
    if (info.Length() == 0) {
        Napi::TypeError::New(env, "One or more integer arguments required").ThrowAsJavaScriptException();
    }
    for (int i = 0; i < info.Length(); i++ ) {
        if (!info[i].IsNumber()) {
            Napi::TypeError::New(env, "Integer arguments only").ThrowAsJavaScriptException();
        }
        flags |= info[i].As<Napi::Number>().Int32Value();  //bitwise OR to set multiple flags
    }
    nft_ctx_output_set_flags(this->ctx_, flags);
    return Napi::Number::New(env, nft_ctx_output_get_flags(this->ctx_));
}

// Define the class methods and values that will be exported by NAPI
Napi::Function LibNftables::GetClass(Napi::Env env) {
    return DefineClass(env, "NapiObjectWrap", {
        InstanceMethod("_runCmd", &LibNftables::runCmd),
        InstanceMethod("_setDryRun", &LibNftables::setDryRun),
        InstanceMethod("_getDryRun", &LibNftables::getDryRun),
        InstanceMethod("_setOutputFlags", &LibNftables::setOutputFlags),
        InstanceMethod("_getOutputFlags", &LibNftables::getOutputFlags),
        InstanceMethod("_initContext", &LibNftables::initContext),
        StaticValue("OUTPUT_DEFAULT", Napi::Number::New(env, 0)),
        StaticValue("OUTPUT_REVERSE_DNS", Napi::Number::New(env, NFT_CTX_OUTPUT_REVERSEDNS)),
        StaticValue("OUTPUT_SERVICE_NAME", Napi::Number::New(env, NFT_CTX_OUTPUT_SERVICE)),
        StaticValue("OUTPUT_STATELESS", Napi::Number::New(env, NFT_CTX_OUTPUT_STATELESS)),
        StaticValue("OUTPUT_HANDLE", Napi::Number::New(env, NFT_CTX_OUTPUT_HANDLE)),
        StaticValue("OUTPUT_JSON", Napi::Number::New(env, NFT_CTX_OUTPUT_JSON)),
        StaticValue("OUTPUT_ECHO", Napi::Number::New(env, NFT_CTX_OUTPUT_ECHO)),
        StaticValue("OUTPUT_GUID", Napi::Number::New(env, NFT_CTX_OUTPUT_GUID)),
        StaticValue("OUTPUT_NUMERIC_PROTOCOL", Napi::Number::New(env, NFT_CTX_OUTPUT_NUMERIC_PROTO)),
        StaticValue("OUTPUT_NUMERIC_PRIORITY", Napi::Number::New(env, NFT_CTX_OUTPUT_NUMERIC_PRIO)),
        StaticValue("OUTPUT_NUMERIC_SYMBOL", Napi::Number::New(env, NFT_CTX_OUTPUT_NUMERIC_SYMBOL)),
        StaticValue("OUTPUT_NUMERIC_TIME", Napi::Number::New(env, NFT_CTX_OUTPUT_NUMERIC_TIME)),
        StaticValue("OUTPUT_NUMERIC_ALL", Napi::Number::New(env, NFT_CTX_OUTPUT_NUMERIC_ALL)),
        StaticValue("OUTPUT_TERSE", Napi::Number::New(env, NFT_CTX_OUTPUT_TERSE))
    });
}

// should be possible to export the constants directly, not as instanceValues...
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::String name = Napi::String::New(env, "LibNftables");
    exports.Set(name, LibNftables::GetClass(env));
    return exports;
}

// Macro to init the module
NODE_API_MODULE(addon, Init)
