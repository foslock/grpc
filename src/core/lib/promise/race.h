// Copyright 2021 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GRPC_SRC_CORE_LIB_PROMISE_RACE_H
#define GRPC_SRC_CORE_LIB_PROMISE_RACE_H

#include <grpc/support/port_platform.h>

#include <utility>

#include "src/core/util/json/json.h"
#include "src/proto/grpc/channelz/v2/promise.upb.h"

namespace grpc_core {

/// Run all the promises, return the first result that's available.
/// If two results are simultaneously available, bias towards the first result
/// listed.
template <typename... Promises>
class Race;

template <typename Promise, typename... Promises>
class Race<Promise, Promises...> {
 public:
  using Result = decltype(std::declval<Promise>()());

  GPR_ATTRIBUTE_ALWAYS_INLINE_FUNCTION explicit Race(Promise promise,
                                                     Promises... promises)
      : promise_(std::move(promise)), next_(std::move(promises)...) {}

  GPR_ATTRIBUTE_ALWAYS_INLINE_FUNCTION Result operator()() {
    // Check our own promise.
    auto r = promise_();
    if (r.pending()) {
      // Check the rest of them.
      return next_();
    }
    // Return the first ready result.
    return std::move(r.value());
  }

  void ToProto(grpc_channelz_v2_Promise* promise_proto,
               upb_Arena* arena) const {
    auto* race_promise =
        grpc_channelz_v2_Promise_mutable_race_promise(promise_proto, arena);
    auto** children = grpc_channelz_v2_Promise_Race_resize_children(
        race_promise, 1 + sizeof...(Promises), arena);
    for (size_t i = 0; i < 1 + sizeof...(Promises); ++i) {
      children[i] = grpc_channelz_v2_Promise_new(arena);
    }
    SetChildrenProto(children, 0, arena);
  }

  void SetChildrenProto(grpc_channelz_v2_Promise** promise_protos, int index,
                        upb_Arena* arena) const {
    PromiseAsProto(promise_, promise_protos[index], arena);
    next_.SetChildrenProto(promise_protos, index + 1, arena);
  }

 private:
  // The Promise checked by this instance.
  Promise promise_;
  // We recursively expand to check the rest of the instances.
  Race<Promises...> next_;
};

template <typename Promise>
class Race<Promise> {
 public:
  using Result = decltype(std::declval<Promise>()());
  GPR_ATTRIBUTE_ALWAYS_INLINE_FUNCTION explicit Race(Promise promise)
      : promise_(std::move(promise)) {}
  GPR_ATTRIBUTE_ALWAYS_INLINE_FUNCTION Result operator()() {
    return promise_();
  }

  void ToProto(grpc_channelz_v2_Promise* promise_proto,
               upb_Arena* arena) const {
    PromiseAsProto(promise_, promise_proto, arena);
  }

  void SetChildrenProto(grpc_channelz_v2_Promise** promise_protos, int index,
                        upb_Arena* arena) const {
    PromiseAsProto(promise_, promise_protos[index], arena);
  }

 private:
  Promise promise_;
};

template <typename... Promises>
Race(Promises...) -> Race<Promises...>;

}  // namespace grpc_core

#endif  // GRPC_SRC_CORE_LIB_PROMISE_RACE_H
