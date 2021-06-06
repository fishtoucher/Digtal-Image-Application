#include <stdio.h>
#include<string>
#include<string.h>
#include<iostream>

using namespace std;

int main()
{
	string a = "0";
	//string num = ".jpg";
	a[0] += 1;
	a += ".jpg";
	cout << a << endl;
}