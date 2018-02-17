oauth2-helper: oauth2-helper.c
	gcc -o $@ $^ -framework CoreFoundation -framework CoreServices
