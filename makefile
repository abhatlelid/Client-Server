make: server.c client.c funksjoner.c
	gcc -Wall -Wextra -Wpedantic -std=gnu99 -g server.c -o server && gcc -Wall -Wextra -Wpedantic -std=gnu99 -g client.c -o client

server: server.c
	gcc -Wall -Wextra -Wpedantic -std=gnu99 -g server.c -o server

client: client.c
	gcc -Wall -Wextra -Wpedantic -std=gnu99 -g client.c -o client

clean:
	rm client server 