/**
 * @file oauth2-helper.c
 *
 * @copyright 2018 Bill Zissimopoulos
 */

/*
 * general definitions
 */
#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>
#undef RtlFillMemory
NTSYSAPI VOID NTAPI RtlFillMemory(VOID *Destination, DWORD Length, BYTE Fill);
#define memset(d, v, s)                 (RtlFillMemory(d, v, s), (d))
#define strcpy(d, s)                    lstrcpyA(d, s)
#define strlen(s)                       lstrlenA(s)
#define malloc(s)                       HeapAlloc(GetProcessHeap(), 0, s)
#define realloc(p, s)                   HeapReAlloc(GetProcessHeap(), 0, p, s)
#define free(p)                         ((p) ? (void)HeapFree(GetProcessHeap(), 0, p) : (void)0)
#define exit(n)                         ExitProcess(n)
#define STDOUT_FILENO                   (intptr_t)GetStdHandle(STD_OUTPUT_HANDLE)
#define STDERR_FILENO                   (intptr_t)GetStdHandle(STD_ERROR_HANDLE)

#elif defined(__CYGWIN__)
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
void *__stdcall ShellExecuteA(
    void *hwnd,
    const char *lpOperation,
    const char *lpFile,
    const char *lpParameters,
    const char *lpDirectory,
    int nShowCmd);
unsigned long __stdcall GetLastError(void);
#define SW_SHOWNORMAL                   1

#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <netinet/in.h>
#include <sys/socket.h>

#elif defined(__linux__)
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;

#else
#error Unknown platform
#endif

/*
 * file I/O
 */
#if defined(_WIN64) || defined(_WIN32)
#define O_RDONLY                        0x0000
#define O_WRONLY                        0x0001
#define O_RDWR                          0x0002
#define O_APPEND                        0x0008
#define O_CREAT                         0x0100
#define O_TRUNC                         0x0200
#define O_EXCL                          0x0400
static inline intptr_t open(const char *path, int oflag)
{
    static DWORD da[] = { GENERIC_READ, GENERIC_WRITE, GENERIC_READ | GENERIC_WRITE, 0 };
    static DWORD cd[] = { OPEN_EXISTING, OPEN_ALWAYS, TRUNCATE_EXISTING, CREATE_ALWAYS };
    DWORD DesiredAccess = 0 == (oflag & O_APPEND) ?
        da[oflag & (O_RDONLY | O_WRONLY | O_RDWR)] :
        (da[oflag & (O_RDONLY | O_WRONLY | O_RDWR)] & ~FILE_WRITE_DATA) | FILE_APPEND_DATA;
    DWORD CreationDisposition = (O_CREAT | O_EXCL) == (oflag & (O_CREAT | O_EXCL)) ?
        CREATE_NEW :
        cd[(oflag & (O_CREAT | O_TRUNC)) >> 8];
    HANDLE h = CreateFileA(path,
        DesiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
        CreationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
    return INVALID_HANDLE_VALUE != h ? (intptr_t)h : -1;
}
static inline int close(intptr_t fd)
{
    return CloseHandle((HANDLE)fd) ? 0 : -1;
}
static inline int read(intptr_t fd, void *buf, size_t len)
{
    DWORD BytesTransferred;
    return ReadFile((HANDLE)fd, buf, len, &BytesTransferred, 0) ? BytesTransferred : -1;
}
static inline int write(intptr_t fd, const void *buf, size_t len)
{
    DWORD BytesTransferred;
    return WriteFile((HANDLE)fd, buf, len, &BytesTransferred, 0) ? BytesTransferred : -1;
}
static inline int file_errno()
{
    return GetLastError();
}

#else /* POSIX */
static inline int file_errno()
{
    return errno;
}
#endif

/*
 * sockets
 */
#if defined(_WIN64) || defined(_WIN32)
typedef int socklen_t;
static int socket_init(void)
{
    static int initdone = 0;
    WSADATA wsadata;
    int status;
    if (initdone)
        return 0;
    status = WSAStartup(MAKEWORD(2, 0), &wsadata);
    if (0 != status)
        return status;
    initdone = 1;
    return 0;
}
static inline int socket_close(SOCKET s)
{
    return closesocket(s);
}
static inline int socket_errno()
{
    return WSAGetLastError();
}

#else
typedef int SOCKET;
#define INVALID_SOCKET                  (-1)
#define SOCKET_ERROR                    (-1)
static int socket_init(void)
{
    return 0;
}
static inline int socket_close(SOCKET s)
{
    return close(s);
}
static inline int socket_errno()
{
    return errno;
}
#endif

enum
{
    E_FILE                              = 'F',
    E_BROWSER                           = 'B',
    E_SERVER                            = 'S',
    E_NETWORK                           = 'N',
    E_TIMEOUT                           = 'T',
};

#define RSP200\
    "HTTP/1.1 200 OK\r\n"\
    "Content-type: text/html\r\n"\
    "\r\n"\
    "<html>"\
    "<body>"\
    "<h1>You are authorized</h1>"\
    "</body>"\
    "</html>"
#define RSP404\
    "HTTP/1.1 404 Not Found\r\n"\
    "Content-type: text/html\r\n"\
    "\r\n"\
    "<html>"\
    "<body>"\
    "<h1>404 Not Found</h1>"\
    "</body>"\
    "</html>"

void err(int result, const char *fmt, ...)
{
    va_list ap;
    char buf[64+1024];
    size_t len;

    switch (result)
    {
    case E_FILE:
        strcpy(buf, "E_FILE: ");
        break;
    case E_BROWSER:
        strcpy(buf, "E_BROWSER: ");
        break;
    case E_SERVER:
        strcpy(buf, "E_SERVER: ");
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
    wvsprintfA(buf + len, fmt, ap);
#else
    vsnprintf(buf + len, sizeof buf - len, fmt, ap);
#endif
    va_end(ap);

    write(STDERR_FILENO, buf, strlen(buf));
}

int load_file(const char *path, char **pp)
{
    int result = E_FILE;
    char *p = 0, *q;
    size_t n = 64 * 1024;
    int fd = -1;

    *pp = 0;

    p = malloc(n);
    if (0 == p)
    {
        err(result, "malloc\n");
        goto exit;
    }

    fd = open(path, O_RDONLY);
    if (-1 == fd)
    {
        err(result, "open: %d\n", file_errno());
        goto exit;
    }

    n = read(fd, p, n - 1);
    if (-1 == n)
    {
        err(result, "read: %d\n", file_errno());
        goto exit;
    }
    p[n++] = '\0';

    q = realloc(p, n);
    if (0 == q)
    {
        err(result, "realloc\n");
        goto exit;
    }

    *pp = q;
    result = 0;

exit:
    if (-1 != fd)
        close(fd);

    if (0 != result)
        free(p);

    return result;
}

int browser(const char *url)
{
    int result = E_BROWSER;

#if defined(_WIN64) || defined(_WIN32) || defined(__CYGWIN__)
    if (!ShellExecuteA(0, "open", url, 0, 0, SW_SHOWNORMAL))
    {
        err(result, "ShellExecuteA: %d\n", GetLastError());
        goto exit;
    }
    result = 0;
exit:

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
	char *argv[] =
	{
		"xdg-open",
        (char *)url,
		0,
	};
    posix_spawn_file_actions_t file_actions_stg, *file_actions = 0;
	pid_t pid;
	int status;
    status = posix_spawn_file_actions_init(&file_actions_stg);
    if (0 != status)
    {
        err(result, "posix_spawn_file_actions_init: %d\n", status);
        goto exit;
    }
    file_actions = &file_actions_stg;
    status = posix_spawn_file_actions_addopen(
        file_actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    if (0 != status)
    {
        err(result, "posix_spawn_file_actions_addopen: %d\n", status);
        goto exit;
    }
    status = posix_spawnp(&pid, argv[0], file_actions, 0, argv, environ);
    if (0 != status)
    {
        err(result, "posix_spawnp: %d\n", status);
        goto exit;
    }
    if (pid != waitpid(pid, &status, 0))
    {
        err(result, "waitpid: %d\n", errno);
        goto exit;
    }
    if (!WIFEXITED(status) || 0 != WEXITSTATUS(status))
    {
        err(result, "xdg-open: %d\n", status);
        goto exit;
    }
    result = 0;
exit:
    if (0 != file_actions)
        posix_spawn_file_actions_destroy(file_actions);
#endif

    return result;
}

int server_socket(unsigned port, unsigned *pport, SOCKET *psocket)
{
    int result = E_SERVER;
    SOCKET s = INVALID_SOCKET;
    struct sockaddr_in addr;
    socklen_t len;
    int status;

    status = socket_init();
    if (0 != status)
    {
        err(result, "socket_init: %d\n", status);
        goto fail;
    }

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == s)
    {
        err(result, "socket: %d\n", socket_errno());
        goto fail;
    }

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);

    status = bind(s, (struct sockaddr *)&addr, sizeof addr);
    if (SOCKET_ERROR == status)
    {
        err(result, "bind: %d\n", socket_errno());
        goto fail;
    }

    status = listen(s, 1);
    if (SOCKET_ERROR == status)
    {
        err(result, "listen: %d\n", socket_errno());
        goto fail;
    }

    len = sizeof addr;
    status = getsockname(s, (struct sockaddr *)&addr, &len);
    if (SOCKET_ERROR == status)
    {
        err(result, "getsockname: %d\n", socket_errno());
        goto fail;
    }

    result = 0;

    *pport = ntohs(addr.sin_port);
    *psocket = s;
    return result;

fail:
    if (INVALID_SOCKET != s)
        socket_close(s);

    *pport = 0;
    *psocket = INVALID_SOCKET;
    return result;
}

int server(SOCKET s, unsigned timeout, char *rsp200)
{
    int result;
    SOCKET a = INVALID_SOCKET;
    struct timeval tv;
    fd_set fds;
    int status, n;
    char req[2048 + 1], *rsp;
    char *resource = 0, *p;

    tv.tv_sec = timeout ? timeout : 120;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(s, &fds);
    status = select(s + 1, &fds, 0, 0, &tv);
    if (SOCKET_ERROR == status)
    {
        err(result = E_SERVER, "select: %d\n", socket_errno());
        goto exit;
    }
    else if (0 == status)
    {
        err(result = E_TIMEOUT, "select\n");
        goto exit;
    }

    a = accept(s, 0, 0);
    if (INVALID_SOCKET == a)
    {
        err(result = E_NETWORK, "accept: %d\n", socket_errno());
        goto exit;
    }

    n = recv(a, req, sizeof req - 1, 0);
    if (SOCKET_ERROR == n)
    {
        err(result = E_NETWORK, "recv: %d\n", socket_errno());
        goto exit;
    }
    req[n] = '\0';

    if ('G' == req[0] && 'E' == req[1] && 'T' == req[2] && req[3] == ' ')
    {
        resource = req + 3;
        *resource = '+';
        for (p = resource + 1; *p && ' ' != *p && '\r' != *p && '\n' != *p; p++)
            ;
        *p++ = '\n';
    }

    rsp = resource ? (rsp200 ? rsp200 : RSP200) : RSP404;
    send(a, rsp, strlen(rsp), 0);

    if (0 == resource)
    {
        err(result = E_NETWORK, "HTTP: no resource\n");
        goto exit;
    }

    write(STDOUT_FILENO, resource, p - resource);

    result = 0;

exit:
    if (INVALID_SOCKET != a)
        socket_close(a);

    return result;
}

static unsigned strtouint(const char *p)
{
    unsigned v;

    for (v = 0; *p; p++)
    {
        int c = *p;

        if ('0' <= c && c <= '9')
            v = 10 * v + (c - '0');
        else
            break;
    }

    return v;
}

void usage(void)
{
    char usage[] = "usage: oauth2-helper [-pPORT][-tTIMEOUT][-FPATH] URL\n";
    write(STDERR_FILENO, usage, strlen(usage));
    exit(2);
}

int main(int argc, char *argv[])
{
    char *urlarg, url[1024];
    unsigned port = 0, timeout = 0;
    const char *rsp200file = 0;
    int argi;
    char *rsp200 = 0;
    SOCKET s;
    int result;

    for (argi = 1; argc > argi; argi++)
    {
        if ('-' != argv[argi][0])
            break;
        switch (argv[argi][1])
        {
        case 'p':
            port = strtouint(argv[argi] + 2);
            break;
        case 't':
            timeout = strtouint(argv[argi] + 2);
            break;
        case 'F':
            rsp200file = argv[argi] + 2;
            break;
        default:
            usage();
            break;
        }
    }
    if (1 != argc - argi)
        usage();

    urlarg = argv[argi];
    if (!('h' == urlarg[0] && 't' == urlarg[1] && 't' == urlarg[2] && 'p' == urlarg[3] &&
        (':' == urlarg[4] || ('s' == urlarg[4] && ':' == urlarg[5]))))
        usage();

    if (0 != rsp200file)
    {
        result = load_file(rsp200file, &rsp200);
        if (0 != result)
            goto fail;
    }

    result = server_socket(port, &port, &s);
    if (0 != result)
        goto fail;

    for (char *p = urlarg; *p; p++)
        if ('%' == *p)
            *p = '\x01';
        else if ('[' == p[0] && ']' == p[1])
        {
            p[0] = '%';
            p[1] = 'd';
        }

#if defined(_WIN64) || defined(_WIN32)
    wsprintfA(url, urlarg, port);
#else
    snprintf(url, sizeof url, urlarg, port);
#endif

    for (char *p = url; *p; p++)
        if ('\x01' == *p)
            *p = '%';
    
    result = browser(url);
    if (0 != result)
        goto fail;

    result = server(s, timeout, rsp200);
    if (0 != result)
        goto fail;

    socket_close(s);
    free(rsp200);

    return 0;

fail:
    {
        char resbuf[3];
        resbuf[0] = '-';
        resbuf[1] = result;
        resbuf[2] = '\n';
        write(STDOUT_FILENO, resbuf, 3);
    }

    return 1;
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
