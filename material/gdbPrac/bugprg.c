#include <stdio.h>

struct time{
	int hour;
	int min;
	int sec;
} gtime = {1,2,3}, gtimes[4];

char* array[4] = {"one","two","three"};

int func(short sw, char* str){
	printf("first char = %c\n", *str);

	if(sw)
		return 2;
	return 3;
}

int main(){
	int lval = 2331, i;
	char* lstr = "I like you.";
	struct time* pt = &gtime;
	for(i = 0; i < 4; ++i){
		printf("loop %d", i);
		gtime.hour += func(i%2,array[i]);
	}
	return 0;
}
