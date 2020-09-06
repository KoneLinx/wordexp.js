/* Copyright (C) 2020 KoneLinx
   This file is part of wordexp.js.

   wordexp.js is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3.0 of the License, or (at your option) any later version.

   wordexp.js is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the wordexp.js repository; if not, see
   <https://www.gnu.org/licenses/>.  */


#include "wordexp.h"

#include <cstdint>
#include <node.h>
#include <utility>

void WordExp(const v8::FunctionCallbackInfo< v8::Value >& args)
{
	v8::Isolate* isolate = args.GetIsolate();
	v8::String::Utf8Value input(isolate, args[0]);
	auto local = v8::String::Empty(isolate);

	Wordexp< std::remove_reference<decltype(**input)>::type > we{ *input };

	if (!we)
	{
		v8::Local<v8::Value> e;
		switch (we.error)
		{
		case wordexp::error::SYNTAX:
			e = v8::Exception::SyntaxError(v8::String::NewFromUtf8Literal(isolate, "Bad syntax: Could not expand input string"));
			break;
		case wordexp::error::BADCHAR:
			e = v8::Exception::SyntaxError(v8::String::NewFromUtf8Literal(isolate, "Bad char: Invalid character was found while expanding input string"));
			break;
		case wordexp::error::BADVAL:
			e = v8::Exception::TypeError(v8::String::NewFromUtf8Literal(isolate, "Internal error: A variable had no reference"));
			break;
		case wordexp::error::CMDSUB:
			e = v8::Exception::SyntaxError(v8::String::NewFromUtf8Literal(isolate, "Internal error: Cmd substitution with no Cmd"));
			break;
		case wordexp::error::NOSPACE:
			e = v8::Exception::WasmRuntimeError(v8::String::NewFromUtf8Literal(isolate, "Internal error: No allocation space left."));
			break;
		default:
			std::string what{ "Internal error: Unknown error " + std::to_string(we.error) };
			v8::String::NewFromUtf8(isolate, what.c_str()).ToLocal(&local);
			e = v8::Exception::WasmRuntimeError(local);
		}
		isolate->ThrowException(e);
		return;
	}

	auto* arr = new v8::Local<v8::Value>[we._wordexp.we_wordc];

	for (unsigned i{}; i < we._wordexp.we_wordc; ++i)
	{
		v8::String::NewFromUtf8(isolate, we._wordexp.we_wordv[i]).ToLocal(&local);
		arr[i] = local.As<v8::Value>();
	}
	args.GetReturnValue().Set(v8::Array::New(isolate, arr, we._wordexp.we_wordc));

	delete[] arr;

}

void Init(v8::Local<v8::Object> exports)
{
	NODE_SET_METHOD(exports, "wordexp", WordExp);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)
