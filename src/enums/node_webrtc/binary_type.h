#pragma once

// IWYU pragma: no_include "src/enums/macros/impls.h"

#define BINARY_TYPE BinaryType
#define BINARY_TYPE_NAME "BinaryType"
#define BINARY_TYPE_LIST \
  ENUM_UNSUPPORTED(BINARY_TYPE, kBlob, "blob", "\"blob\" is not supported; see TODO") \
  ENUM_SUPPORTED(BINARY_TYPE, kArrayBuffer, "arraybuffer")

#define ENUM(X) BINARY_TYPE ## X
#include "src/enums/macros/def.h"
#include "src/enums/macros/decls.h"
#undef ENUM