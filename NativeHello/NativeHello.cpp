#include "NativeHello.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "NativeHello", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "NativeHello", __VA_ARGS__))

/* This trivial function returns the platform ABI for which this dynamic native library is compiled.*/
const char * getPlatformABI()
{
#if defined(__arm__)
#	if defined(__ARM_ARCH_7A__)
#		if defined(__ARM_NEON__)
#			define ABI "armeabi-v7a/NEON"
#		else
#			define ABI "armeabi-v7a"
#		endif
#	else
#		define ABI "armeabi"
#	endif
#elif defined(__aarch64__)
#	define ABI "arm64-v8a"
#elif defined(__i386__)
#	define ABI "x86"
#elif defined(__x86_64__)
#	define ABI "x86_64"
#else
#	define ABI "unknown"
#endif
	LOGI("This dynamic shared library is compiled with ABI: " ABI ".");
	return "This native library is compiled with ABI: " ABI ".";
}
