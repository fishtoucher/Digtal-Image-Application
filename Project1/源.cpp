#include <stdio.h>

int main()
{
	int a;
	scanf_s("%d%c", &a);
	char b;
	b = getchar();
	printf("%c\n", b);
}