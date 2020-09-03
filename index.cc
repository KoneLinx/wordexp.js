
#include "./wordexp.h"

#include <cstdint>
#include <node.h>

void WordExp(const v8::FunctionCallbackInfo< v8::Value >& args)
{
	v8::Isolate* isolate = args.GetIsolate();
	v8::String::Utf8Value input(isolate, args[0]);
	auto local = v8::String::Empty(isolate);

	wordexp_t e;
	int error = wordexp(*input, &e, 0); // the (magi)C function
	auto* arr = new v8::Local<v8::Value>[e.we_wordc];

	if (error)
	{
		v8::Local<v8::Value> e;
		switch (error)
		{
		case WRDE_SYNTAX:
			e = v8::Exception::SyntaxError(v8::String::NewFromUtf8Literal(isolate, "Bad syntax: Could not expand input string"));
			break;
		case WRDE_BADCHAR:
			e = v8::Exception::SyntaxError(v8::String::NewFromUtf8Literal(isolate, "Bad char: Invalid character was found while expanding input string"));
			break;
		case WRDE_BADVAL:
			e = v8::Exception::TypeError(v8::String::NewFromUtf8Literal(isolate, "Internal error: A variable had no reference"));
			break;
		case WRDE_CMDSUB:
			e = v8::Exception::SyntaxError(v8::String::NewFromUtf8Literal(isolate, "Internal error: Cmd substitution with no Cmd"));
			break;
		case WRDE_NOSPACE:
			e = v8::Exception::WasmRuntimeError(v8::String::NewFromUtf8Literal(isolate, "Internal error: No allocation space left."));
		}
		isolate->ThrowException(e);
		goto cleanup;
	}

	for (unsigned i{}; i < e.we_wordc; ++i)
	{
		v8::String::NewFromUtf8(isolate, e.we_wordv[i]).ToLocal(&local);
		arr[i] = local.As<v8::Value>();
	}
	args.GetReturnValue().Set(v8::Array::New(isolate, arr, e.we_wordc));

cleanup: // Cleanup, never skip this
	delete[] arr;
	wordfree(&e);

}

void Init(v8::Local<v8::Object> exports)
{
	NODE_SET_METHOD(exports, "wordexp", WordExp);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)
