// Copyright 2025 gRPC authors.
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

#include "tools/codegen/core/generate_trace_flags.h"

#include <string>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "third_party/yamlcpp/include/yaml-cpp/yaml.h"

namespace grpc_generator {

namespace {

struct TraceFlag {
  std::string name;
  std::string description;
  bool internal = false;
  bool debug_only = false;
};

std::vector<TraceFlag> ParseTraceFlags(
    const std::vector<std::string>& trace_flags_yaml_paths) {
  std::vector<TraceFlag> trace_flags;
  for (const auto& trace_flags_yaml_path : trace_flags_yaml_paths) {
    YAML::Node config = YAML::LoadFile(trace_flags_yaml_path);
    if (!config.IsMap()) {
      LOG(FATAL) << "Failed to parse trace flags yaml file: "
                 << trace_flags_yaml_path;
    }
    for (const auto& pair : config) {
      if (!pair.first.IsScalar()) continue;
      std::string name = pair.first.as<std::string>();
      std::string description;
      bool internal = false;
      bool debug_only = false;
      if (pair.second.IsMap()) {
        if (pair.second["description"].IsDefined() &&
            pair.second["description"].IsScalar()) {
          description = pair.second["description"].as<std::string>();
        }
        if (pair.second["internal"].IsDefined() &&
            pair.second["internal"].IsScalar()) {
          internal = pair.second["internal"].as<bool>();
        }
        if (pair.second["debug_only"].IsDefined() &&
            pair.second["debug_only"].IsScalar()) {
          debug_only = pair.second["debug_only"].as<bool>();
        }
      } else {
        LOG(FATAL) << "Failed to parse trace flags yaml file: "
                   << trace_flags_yaml_path;
      }
      trace_flags.push_back(
          {std::move(name), std::move(description), internal, debug_only});
    }
  }
  return trace_flags;
}

}  // namespace

std::string GenerateHeader(const std::vector<std::string>& trace_flags_yaml) {
  std::vector<TraceFlag> trace_flags = ParseTraceFlags(trace_flags_yaml);
  if (trace_flags.empty()) {
    return "";
  }
  std::vector<TraceFlag> debug_flags;
  std::vector<TraceFlag> non_debug_flags;
  for (const auto& flag : trace_flags) {
    if (flag.debug_only) {
      debug_flags.push_back(flag);
    } else {
      non_debug_flags.push_back(flag);
    }
  }

  std::string header_guard = "GRPC_SRC_CORE_LIB_DEBUG_TRACE_FLAGS_H";
  std::string result =
      "// Copyright 2024 gRPC authors.\n"
      "//\n"
      "// Licensed under the Apache License, Version 2.0 (the \"License\");\n"
      "// you may not use this file except in compliance with the License.\n"
      "// You may obtain a copy of the License at\n"
      "//\n"
      "//     http://www.apache.org/licenses/LICENSE-2.0\n"
      "//\n"
      "// Unless required by applicable law or agreed to in writing, software\n"
      "// distributed under the License is distributed on an \"AS IS\" "
      "BASIS,\n"
      "// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or "
      "implied.\n"
      "// See the License for the specific language governing permissions and\n"
      "// limitations under the License.\n"
      "\n"
      "//\n"
      "// Automatically generated by "
      "tools/codegen/core/gen_trace_flags.py\n"
      "//\n"
      "\n"
      "#ifndef GRPC_SRC_CORE_LIB_DEBUG_TRACE_FLAGS_H\n"
      "#define GRPC_SRC_CORE_LIB_DEBUG_TRACE_FLAGS_H\n"
      "\n"
      "#include \"src/core/lib/debug/trace_impl.h\"\n"
      "\n"
      "namespace grpc_core {\n"
      "\n";
  for (const auto& flag : debug_flags) {
    absl::StrAppend(&result, "extern DebugOnlyTraceFlag ", flag.name,
                    "_trace;\n");
  }
  for (const auto& flag : non_debug_flags) {
    absl::StrAppend(&result, "extern TraceFlag ", flag.name, "_trace;\n");
  }
  absl::StrAppend(&result, "\n}  // namespace grpc_core\n\n#endif  // ",
                  header_guard, "\n");
  return result;
}

std::string GenerateCpp(const std::vector<std::string>& trace_flags_yaml) {
  std::vector<TraceFlag> trace_flags = ParseTraceFlags(trace_flags_yaml);
  if (trace_flags.empty()) {
    return "";
  }
  std::vector<TraceFlag> debug_flags;
  std::vector<TraceFlag> non_debug_flags;
  for (const auto& flag : trace_flags) {
    if (flag.debug_only) {
      debug_flags.push_back(flag);
    } else {
      non_debug_flags.push_back(flag);
    }
  }

  std::string result =
      "// Copyright 2024 gRPC authors.\n"
      "//\n"
      "// Licensed under the Apache License, Version 2.0 (the \"License\");\n"
      "// you may not use this file except in compliance with the License.\n"
      "// You may obtain a copy of the License at\n"
      "//\n"
      "//     http://www.apache.org/licenses/LICENSE-2.0\n"
      "//\n"
      "// Unless required by applicable law or agreed to in writing, software\n"
      "// distributed under the License is distributed on an \"AS IS\" "
      "BASIS,\n"
      "// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or "
      "implied.\n"
      "// See the License for the specific language governing permissions and\n"
      "// limitations under the License.\n"
      "\n"
      "//\n"
      "// Automatically generated by "
      "tools/codegen/core/gen_trace_flags.py\n"
      "//\n"
      "\n"
      "#include \"src/core/lib/debug/trace.h\"\n"
      "#include \"src/core/util/no_destruct.h\"\n"
      "\n"
      "namespace grpc_core {\n\n";

  auto generate_definitions = [&](const std::vector<TraceFlag>& flags,
                                  bool is_debug) {
    for (const auto& flag : flags) {
      const std::string declaration =
          is_debug ? "DebugOnlyTraceFlag " : "TraceFlag ";
      const std::string flag_name_and_trace = flag.name + "_trace";
      if (declaration.length() + flag_name_and_trace.length() + 10 > 80) {
        absl::StrAppend(&result, declaration, flag_name_and_trace,
                        "(false,\n    \"", flag.name, "\");\n");
      } else {
        absl::StrAppend(&result, declaration, flag_name_and_trace, "(false, \"",
                        flag.name, "\");\n");
      }
    }
  };
  generate_definitions(debug_flags, true);
  generate_definitions(non_debug_flags, false);

  result +=
      "\n"
      "const absl::flat_hash_map<std::string, TraceFlag*>& "
      "GetAllTraceFlags() {\n"
      "  static const NoDestruct<absl::flat_hash_map<std::string, "
      "TraceFlag*>> all(\n"
      "      absl::flat_hash_map<std::string, TraceFlag*>({\n";
  bool first = true;
  auto append_map_entries = [&](const std::vector<TraceFlag>& flags) {
    for (const auto& flag : flags) {
      if (!first) result += "\n";
      first = false;
      absl::StrAppend(&result, "          {\"", flag.name, "\", &", flag.name,
                      "_trace},");
    }
  };
  append_map_entries(non_debug_flags);
  if (!debug_flags.empty()) {
    result += "\n";
    result += "#ifndef NDEBUG\n";
    first = true;
    append_map_entries(debug_flags);
    result += "\n#endif";
  }
  result +=
      "\n      }));\n"
      "  return *all;\n"
      "}\n\n"
      "}  // namespace grpc_core\n";
  return result;
}

std::string GenerateMarkdown(const std::vector<std::string>& trace_flags_yaml) {
  std::vector<TraceFlag> trace_flags = ParseTraceFlags(trace_flags_yaml);
  if (trace_flags.empty()) {
    return "";
  }
  std::vector<TraceFlag> debug_flags;
  std::vector<TraceFlag> non_debug_flags;
  for (const auto& flag : trace_flags) {
    if (flag.internal) continue;
    if (flag.debug_only) {
      debug_flags.push_back(flag);
    } else {
      non_debug_flags.push_back(flag);
    }
  }

  std::string result =
      R"(<!---
Automatically generated by tools/codegen/core/gen_trace_flags.py
--->

gRPC Trace Flags
----------------

The `GRPC_TRACE` environment variable supports a comma-separated list of tracer
names or glob patterns that provide additional insight into how gRPC C core is
processing requests via debug logs. Available tracers include:

)";
  auto append_flags = [&](const std::vector<TraceFlag>& flags) {
    for (const auto& flag : flags) {
      absl::StrAppend(&result, "  - ", flag.name, " - ", flag.description,
                      "\n");
    }
  };
  append_flags(non_debug_flags);
  if (!debug_flags.empty()) {
    result +=
        R"(
The following tracers will only run in binaries built in DEBUG mode. This is
accomplished by invoking `bazel build --config=dbg <target>`
)";
    append_flags(debug_flags);
  }
  result += R"(
Glob patterns and special cases:
  - `*` can be used to turn all traces on.
  - Individual traces can be disabled by prefixing them with `-`.
  - `*refcount*` will turn on all of the tracers for refcount debugging.
  - if `list_tracers` is present, then all of the available tracers will be
    printed when the program starts up.

Example:
export GRPC_TRACE=*,-pending_tags
)";
  return result;
}
}  // namespace grpc_generator
