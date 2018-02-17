/**
 * @file oauth2-helper.c
 *
 * @copyright 2018 Bill Zissimopoulos
 */

#include <stdarg.h>

#if defined(_WIN64) || defined(_WIN32)
#elif defined(__CYGWIN__)
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#elif defined(__linux__)
#else
#error Unknown platform
#endif

enum
{
    E_BROWSER                           = 'B',
    E_NETWORK                           = 'N',
    E_TIMEOUT                           = 'T',
};

void err(int result, const char *fmt, ...)
{
    va_list ap;
    char buf[1024];
    size_t len;

    switch (result)
    {
    case E_BROWSER:
        strcpy(buf, "E_BROWSER: ");
        break;
    case E_NETWORK:
        strcpy(buf, "E_NETWORK: ");
        break;
    case E_TIMEOUT:
        strcpy(buf, "E_TIMEOUT: ");
        break;
    default:
        strcpy(buf, "E_UNKNOWN: ");
        break;
    }
    len = strlen(buf);

    va_start(ap, fmt);
    vsnprintf(buf + len, sizeof buf - len, fmt, ap);
    va_end(ap);

    write(2, buf, strlen(buf));
}

int browser(const char *url)
{
#if defined(_WIN64) || defined(_WIN32)
#elif defined(__CYGWIN__)
#elif defined(__APPLE__)
    CFStringRef urlstr = 0;
    CFURLRef urlref = 0;
    OSStatus status;
    int result = E_BROWSER;
    urlstr = CFStringCreateWithCString(0, url, kCFStringEncodingUTF8);
    if (0 == urlstr)
    {
        err(result, "CFStringCreateWithCString\n");
        goto exit;
    }
    urlref = CFURLCreateWithString(0, urlstr, 0);
    if (0 == urlstr)
    {
        err(result, "CFURLCreateWithString\n");
        goto exit;
    }
    status = LSOpenCFURLRef(urlref, 0);
    if (noErr != status)
    {
        err(result, "LSOpenCFURLRef: %d\n", status);
        goto exit;
    }
    result = 0;
exit:
    if (0 != urlref)
        CFRelease(urlref);
    if (0 != urlstr)
        CFRelease(urlstr);
    return result;
#elif defined(__linux__)
#else
#error Unknown platform
#endif
}

int main(int argc, char *argv[])
{
    browser(argv[1]);
    return 0;
}
