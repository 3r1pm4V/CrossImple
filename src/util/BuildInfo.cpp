#include "BuildInfo.h"

#ifndef CROSSINK_VERSION
#define CROSSINK_VERSION "unknown"
#endif
#ifndef CROSSINK_GIT_SHA
#define CROSSINK_GIT_SHA "unknown"
#endif
#ifndef CROSSINK_GIT_DIRTY
#define CROSSINK_GIT_DIRTY "unknown"
#endif

#ifndef CROSSINK_SUPPORT_VERSION
#define CROSSINK_SUPPORT_VERSION "unknown"
#endif
#ifndef CROSSINK_SDK_SHA
#define CROSSINK_SDK_SHA "unknown"
#endif

namespace AppVersion {
const char* version() { return CROSSINK_VERSION; }
const char* versionLabel() { return "CrossImple " CROSSINK_VERSION; }
const char* userAgent() { return "CrossImple-ESP32-" CROSSINK_VERSION; }
const char* supportVersion() { return CROSSINK_SUPPORT_VERSION; }
const char* sdkSha() { return CROSSINK_SDK_SHA; }
const char* gitSha() { return CROSSINK_GIT_SHA; }
const char* gitDirtyFlag() { return CROSSINK_GIT_DIRTY; }
}  // namespace AppVersion
