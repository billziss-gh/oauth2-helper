/**
 * @file oauth2-helper.c
 *
 * @copyright 2018 Bill Zissimopoulos
 */

#include <stdarg.h>

#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>
#define strcpy(d, s)                    lstrcpyA(d, s)
#define strlen(s)                       lstrlenA(s)
#define malloc(s)                       HeapAlloc(GetProcessHeap(), 0, s)
#define free(p)                         ((p) ? (void)HeapFree(GetProcessHeap(), 0, p) : (void)0)
#define STDOUT_FILENO                   (intptr_t)GetStdHandle(STD_OUTPUT_HANDLE)
#define STDERR_FILENO                   (intptr_t)GetStdHandle(STD_ERROR_HANDLE)
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

#if defined(_WIN64) || defined(_WIN32)
int write(intptr_t fd, const void *buf, size_t len)
{
    DWORD BytesTransferred;
    return WriteFile((HANDLE)fd, buf, len, &BytesTransferred, 0) ? BytesTransferred : -1;
}
#endif

void err(int result, const char *fmt, ...)
{
    va_list ap;
    char buf[64+1024];
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
#if defined(_WIN64) || defined(_WIN32)
    wvsprintf(buf + len, fmt, ap);
#else
    vsnprintf(buf + len, sizeof buf - len, fmt, ap);
#endif
    va_end(ap);

    write(STDERR_FILENO, buf, strlen(buf));
}

int browser(const char *url)
{
    int result = E_BROWSER;
#if defined(_WIN64) || defined(_WIN32)
    if (!ShellExecuteA(0, "open", url, 0, 0, SW_SHOWNORMAL))
    {
        err(result, "ShellExecuteA: %d\n", GetLastError());
        goto exit;
    }
    result = 0;
exit:
#elif defined(__CYGWIN__)
#elif defined(__APPLE__)
    CFStringRef urlstr = 0;
    CFURLRef urlref = 0;
    OSStatus status;
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
#elif defined(__linux__)
#endif
    return result;
}

int main(int argc, char *argv[])
{
    browser(argv[1]);
    return 0;
}

#if defined(_WIN64) || defined(_WIN32)
void mainCRTStartup(void)
{
    DWORD Argc;
    PWSTR *Argv;
    int Length;
    char **argv, *argp, *argendp;

    Argv = CommandLineToArgvW(GetCommandLineW(), &Argc);
    if (0 == Argv)
        ExitProcess(GetLastError());
    
    Length = 0;
    for (DWORD I = 0; Argc > I; I++)
        Length += WideCharToMultiByte(CP_UTF8, 0, Argv[I], -1, 0, 0, 0, 0);
    argv = malloc((Argc + 1) * sizeof(char *) + Length);
    argp = (char *)argv + (Argc + 1) * sizeof(char *);
    argendp = argp + Length;
    for (DWORD I = 0; Argc > I; I++)
    {
        argv[I] = argp;
        argp += WideCharToMultiByte(CP_UTF8, 0, Argv[I], -1, argp, argendp - argp, 0, 0);
    }
    argv[Argc] = 0;

    LocalFree(Argv);

    ExitProcess(main(Argc, argv));
}
#endif
