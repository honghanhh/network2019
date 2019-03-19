#include<stdio.h>
#include<netdb.h>
#include<arpa/inet.h>

int main(){
	
	char str[100];
	//Input
	printf("%s\n", " Please enter domain name:");
	scanf("%s", str);

    //Gethostbyname
	struct hostent *host_name = gethostbyname(str);


	//Output
	printf("%s\n", " IP address result:");
	struct in_addr **ip_addr ;
	ip_addr = (struct in_addr **)host_name->h_addr_list;

	for (int i=0;ip_addr[i] != NULL; i++){
		printf("%s\n", inet_ntoa(*ip_addr[i]));
	}
}