all: clean
	gcc main.c -g -lncursesw -ltinfow -lreadline -o a.out

clean:
	rm a.out || true

test: 
	gcc test.c -g -lncursesw -ltinfow -lmenu -lreadline -o a.out

install:
	cp ./a.out ~/ctrlr

docker_debian:
	docker build -t ctrlr . && docker run --rm -v `pwd`:/mnt ctrlr make debian

debian: clean
	/usr/bin/gcc main.c -g -lncursesw -lreadline -o a.out
