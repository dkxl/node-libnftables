#ifndef SRC_NAPI_LIBNFTABLES_H_
#define SRC_NAPI_LIBNFTABLES_H_

#include <napi.h>

class LibNftables : public Napi::ObjectWrap<LibNftables> {
public:
    static Napi::Function GetClass(Napi::Env env);
    LibNftables(const Napi::CallbackInfo& info);
    ~LibNftables();

private:
    void initContext(const Napi::CallbackInfo& info);
    Napi::Value runCmd(const Napi::CallbackInfo& info);
    Napi::Value setOutputFlags(const Napi::CallbackInfo& info);
    Napi::Value getOutputFlags(const Napi::CallbackInfo& info);
    Napi::Value setDryRun(const Napi::CallbackInfo& info);
    Napi::Value getDryRun(const Napi::CallbackInfo& info);

    struct nft_ctx *ctx_;
};

#endif // SRC_NAPI_LIBNFTABLES_H_
