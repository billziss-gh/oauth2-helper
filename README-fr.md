# oauth2-helper - OAuth 2.0 pour les applications natives

Oauth2-helper est un programme simple qui facilite la mise en œuvre d'OAuth2 pour les applications natives selon la RFC [RFC 8252](https://tools.ietf.org/html/rfc8252). Il est écrit en C et fonctionne sous Windows, Cygwin, macOS et Linux.

## Comment utiliser

Oauth2-helper implémente les étapes (1) à (4) de la RFC 8252 :

> (1)  L'application client ouvre un onglet de navigateur avec la demande d'autorisation.
>
> (2)  Le point de terminaison d'autorisation reçoit la demande d'autorisation,
>      authentifie l'utilisateur et obtient l'autorisation.
>      L'authentification de l'utilisateur peut impliquer un chaînage vers d'autres
>      systèmes d'authentification.
>
> (3)  Le serveur d'autorisation envoie un code d'autorisation à
>      l'URI de redirection.
>
> (4)  Le client reçoit le code d'autorisation de l'URI de redirection.
>
> (5)  L'application client présente le code d'autorisation au point
>      de terminaison du jeton.
>
> (6)  Le point de terminaison du jeton valide le code d'autorisation et émet
>      les jetons demandés.

Oauth2-helper est un exécutable simple avec l'utilisation de la ligne de commande suivante :

```
utilisation : oauth2-helper [-pPORT][-tTIMEOUT][-FPATH] URL
```	

Oauth2-helper ouvrira le navigateur système pointé vers l'URL (étape (1)) et écoutera sur un port localhost la réponse, qui arrivera via une redirection de navigateur (étape (4)). L'URL doit contenir un paramètre redirect_uri (conformément à la spécification OAuth2), qui pointe vers localhost; le redirect_uri doit également contenir un port de destination. Oauth2-helper remplacera la chaîne spéciale [] par le port de destination dans l'URL.

Option :

- `-pPORT`: Ecoute sur un PORT spécifique plutôt qu'un port aléatoire. Utile pour les serveurs d'autorisation Oauth2 qui nécessitent que le redirect_uri corresponde exactement.
- `-tTIMEOUT`: Spécifiez un TIMEOUT pour attendre la réponse (la valeur par défaut est de 120 secondes).
- `-FPATH`: Spécifiez un PATH pour un fichier dont le contenu sera rendu dans le navigateur une fois la réponse reçue. Ce fichier doit contenir la réponse HTTP complète et les retours à la ligne doivent être de type CRLF (requis par HTTP). Par exemple:
    ```	
	HTTP/1.1 200 OK
	Content-type: text/html

    <html>
    <body>SUCCES</body>
    </html>
    ```	

Oauth2-helper utilise la sortie standard pour rapporter les résultats. Un rapport de réussite a le format ci-dessous. Le RESOURCE-PATH peut être analysé pour extraire le code d'autorisation de sa chaîne de requête.

```
+RESOURCE-PATH                  # redirect_uri PATH
```

Un rapport d'erreur a le format ci-dessous. Le code d'erreur C peut être F pour une erreur de fichier, B pour une erreur de navigateur, S pour une erreur de socket (écoute), N pour une erreur de réseau et T pour un délai d'attente.

```
-C                              # l'un des F, B, S, N, T
```

Oauth2-helper rapportera des informations d'erreur supplémentaires à la sortie d'erreur standard, mais ces informations n'ont pas été standardisées pour une analyse facile (par exemple, les codes d'erreur sont spécifiques au système). Ces informations d'erreur sont principalement conçues comme une aide au débogage.

Exemples:

```
# ouvrez le navigateur système à https://SERVER/AUTHORIZE et écoutez sur un port aléatoire
$ ./oauth2-helper 'https://SERVER/AUTHORIZE?response_type=code&client_id=CLIENT_ID&scope=SCOPE&redirect_uri=http://localhost:[]'
+/?code=CODE

# ouvrez le navigateur système sur https://SERVER/AUTHORIZE et écoutez sur le port 20000
$ ./oauth2-helper -p20000 'https://SERVER/AUTHORIZE?response_type=code&client_id=CLIENT_ID&scope=SCOPE&redirect_uri=http://localhost:[]'
+/?code=CODE

# exemple concret pour l'application TOMI, ouvre l'explorateur système à l'adresse https://sgconnect-hom.fr.world.socgen/sgconnect/oauth2/authorize puis écoute le port 12345
$ ./oauth2-helper -p12345 -t120 -F./RSP200.html 'https://sgconnect-hom.fr.world.socgen/sgconnect/oauth2/authorize?response_type=code&scope=app.tomi.v1+openid+profile&client_id=46ac2dfd-f82e-4c52-ae6c-8ae089201c6a&redirect_uri=http://127.0.0.1:[]/'
+/?code=CODE
```

## Comment construire

Utilisez `nmake` sous Windows et `make` sur toutes les autres plates-formes.
