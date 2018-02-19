# oauth2-helper - OAuth 2.0 for Native Apps

Oauth2-helper is a simple program that facilitates the implementation of OAuth2 for Native Apps according to [RFC 8252](https://tools.ietf.org/html/rfc8252). It is written in C and runs on Windows, Cygwin, macOS and Linux.

## How to use

Oauth2-helper implements steps (1) through (4) from RFC 8252:

> (1)  Client app opens a browser tab with the authorization request.
>
> (2)  Authorization endpoint receives the authorization request,
>      authenticates the user, and obtains authorization.
>      Authenticating the user may involve chaining to other
>      authentication systems.
>
> (3)  Authorization server issues an authorization code to the
>      redirect URI.
>
> (4)  Client receives the authorization code from the redirect URI.
>
> (5)  Client app presents the authorization code at the token
>      endpoint.
>
> (6)  Token endpoint validates the authorization code and issues the
>      tokens requested.

Oauth2-helper is a simple executable with the following command line usage:

```
usage: oauth2-helper [-pPORT][-tTIMEOUT][-FPATH] URL
```

Oauth2-helper will open the system browser pointed to `URL` (step (1)) and will listen on a localhost port for the response, which will arrive via a browser redirect (step (4)). The `URL` must contain a `redirect_uri` parameter (as per the OAuth2 spec), that points to localhost; the `redirect_uri` must also contain a destination port. Oauth2-helper will replace the special string `[]` with the destination port in `URL`.

Options:

- `-pPORT`: Listen on specific `PORT` rather than a random one. Useful for Oauth2 authorization servers that require the `redirect_uri` to match exactly.
- `-tTIMEOUT`: Specify a `TIMEOUT` to wait for the response (default is 120 seconds).
- `-FPATH`: Specify a `PATH` for a file the contents of which will be rendered in the browser once the response is received. This file must contain the **full** HTTP response. For example:
    ```
    HTTP/1.1 200 OK
    Content-type: text/html

    <html>
    <body>SUCCESS</body>
    </html>
    ```

Oauth2-helper uses the standard output to report results. A success report has the format below. The `RESOURCE-PATH` can be parsed to extract the authorization code from its query string.

```
+RESOURCE-PATH                  # redirect_uri PATH
```

An error report has the format below. The error code `C` can be `F` for a file error, `B` for a browser error, `S` for a server (listening) error, `N` for a network error and `T` for a timeout.

```
-C                              # one of F, B, S, N, T
```

Oauth2-helper will report additional error information at the standard error output, but this information has not been standardized for easy parsing (e.g. error codes are system specific). This error information is primarily intended as a debugging aid.

Examples:

```
# open the system browser at https://SERVER/AUTHORIZE and listen on a random port
$ ./oauth2-helper 'https://SERVER/AUTHORIZE?response_type=code&client_id=CLIENT_ID&scope=SCOPE&redirect_uri=http://localhost:[]'
+
/?code=CODE

# open the system browser at https://SERVER/AUTHORIZE and listen on port 20000
$ ./oauth2-helper -p20000 'https://SERVER/AUTHORIZE?response_type=code&client_id=CLIENT_ID&scope=SCOPE&redirect_uri=http://localhost:[]'
+
/?code=CODE
```

## How to build

Use `nmake` on Windows and `make` on all other platforms.
