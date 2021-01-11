all: clean libs
	gcc main.c menu.o -g -lncursesw -ltinfow -lreadline -o a.out

clean:
	rm a.out || true
	rm menu.o || true

test: 
	gcc test.c -g -lncursesw -ltinfow -lmenu -lreadline -o a.out

install:
	cp ./a.out ~/ctrlr

docker_debian:
	docker build -t ctrlr . && docker run --rm -v `pwd`:/mnt ctrlr make debian

debian: clean
	/usr/bin/gcc main.c -g -lncursesw -lreadline -o a.out

libs:
	gcc -c menu.c -o menu.o
