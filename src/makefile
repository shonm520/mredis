src = $(wildcard ./*c)
obj = $(patsubst ./%.c, ./%.o, $(src))

target = redis.x
CC = gcc
CFLAGS := -g -Wall
CXXFLAGS := -lpthread



%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@ 

$(target):$(obj)
	gcc -o $@ $^ $(CXXFLAGS)



.PHONY:clean
clean:
	rm -f $(obj) $(target) 
.PHONY:cleantemp
cleantemp:
	rm -f $(obj) 