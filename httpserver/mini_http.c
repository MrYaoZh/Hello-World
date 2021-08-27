#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>


#define SERVER_PORT 80

perror_exit(const char *des)
{
	fprintf(stderr, "%s error, reson:%s\n", des, strerror(errno));
	//	close(sock);
	exit(1);
}

void ChangeChar(char* buff, int len);
int	 get_line(int sock, char *buf, int size);
void do_http_request(int sock);
void do_http_response(int sock);
void bad_request(int sock);		//400
void unimplemented(int sock);	//501
void not_found(int sock);		//404
void not_found_do(int sock);		//404
void do_http_response_assign(int sock, const char *url);

int main(void)
{
	int sock;
	struct sockaddr_in server_addr;
	char server_ip[64];
	
	//创建socket
	sock = socket(AF_INET, SOCK_STREAM,0);
	
	if (sock == -1)
	{
		fprintf(stderr, "create socket error, reson:%s\n", strerror(errno));
		
		exit(1);
		
		return 0;
	}

	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;	//select protocl ipv4选择协议族IPV4
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//listen all the ip address监听所有地址
	//server_addr.sin_addr.s_addr = inet_addr("192.168.253.131");//listen the asked ip address监听指定
	server_addr.sin_port = htons(SERVER_PORT);	//bind the port绑定端口

	printf("Server ip:%s\t port:%d\n",
				inet_ntop(AF_INET, &server_addr.sin_addr.s_addr,server_ip,sizeof(server_ip)),
				ntohs(server_addr.sin_port));
	
	int ret = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
	
	if (ret == -1)
	{
		//fprintf(stderr, "bind socket error, reson:%s\n", strerror(errno));
		//close(sock);
		//exit(1);
		close(sock);
		perror_exit("bind");
		return 0;
	}

	ret = listen(sock, 128);	//同时监听128个socket
	
	if (ret == -1)
	{
		fprintf(stderr, "listen socket error, reson:%s\n", strerror(errno));
		close(sock);
		exit(1);
		
		return 0;
	}

	printf("等待客户端的连接.....\n");

	int done = 1;

	while(done)
	{
		struct sockaddr_in client;
		int client_sock, len;
		char client_ip[64];
		char buf[256];
		

		socklen_t client_addr_len;
		client_addr_len = sizeof(client);
		
		printf("等待接收....\n");
		client_sock=accept(sock, (struct sockaddr *)&client, &client_addr_len);
		
		if (client_sock == -1)
		{
			fprintf(stderr, "accept socket error, reson:%s\n", strerror(errno));
			close(sock);
			exit(1);
			
			return 0;
		}
		
		printf("client ip:%s\t port:%d\n",
				inet_ntop(AF_INET, &client.sin_addr.s_addr,client_ip,sizeof(client_ip)),
				ntohs(client.sin_port));
				

		do_http_request(client_sock);
		
		
		char htmlbuf[] = "<html lang='zh-CN'><head>\
    <title>Test</title></head><body><h1>This is a Test HTML!</h1><p>测试网页！</p></body></html>";
		//len = write(client_sock, htmlbuf, sizeof(htmlbuf));
		//printf("Write finished. len:%d\n",len);
		//close(client_sock);
		
	}
	
	

	return 0;
}

void ChangeChar(char* buff, int len)
{
	printf("ChangeChar start[%d]:%s\n", len, buff);
	//char 	*sbuf;
	int 	i;
	for (i =0; i< len; i++)
	{
		if ((*buff >= 'a') && (*buff <= 'z'))
		{
			*buff = *buff-32;
		}
		printf("Change[%d]:%s\n", i, buff);
		buff++;
	}
	//buff = sbuf;
	printf("ChangeChar end[%d]:%s\n", len, buff);
	//return sbuf;
}

void do_http_request(int sock)
{
	//读取客户端发送的http请求
	//1、读取请求行
	int len =0;
	char buf[256];
	char method[16];
	char url[256];
	char path[256];
	struct stat st;
	
	len = get_line(sock, buf, sizeof(buf));
	printf("read line：%s \n", buf);
	if (len > 0)
	{
		int i = 0, j=0;
		while(!isspace(buf[j]) && (i< sizeof(method)-1))
		{
			method[i] = buf[j];
			i++;
			j++;
		}
		method[i] = '\0';
		
		j++;
		//判断方法是否合法
		if(strncasecmp(method, "GET", i) == 0)
		{
			printf("requst = %s, i = %d, j = %d\n", method, i, j);
			
			i = 0;
			while(!isspace(buf[j]) && (i< sizeof(url)-1))
			{
				url[i] = buf[j];
				i++;
				j++;
			}
			url[i] = '\0';
			printf("url = %s, i = %d, j = %d\n", url, i, j);
			
			sprintf(path, "./html_docs%s", url);
			
			if (path[strlen(path) - 1] == '/')
			{
				strcat(path, "index.html");
			}
			
			printf("path = %s\n", path);
			
			do
			{
				len = get_line(sock, buf, sizeof(buf));
				printf("read line：%s \n", buf);
			}while(len > 0);
			
			if (stat(path, &st) == -1)
			{		//文件不存在或读取异常
				not_found_do(sock);
			}
			else	//文件存在
			{
				if (S_ISDIR(st.st_mode))	//是文件夹
				{
					strcat(path, "/index.html");
				}
				printf(" last path = %s\n",path);
				do_http_response_assign(sock, path);
				//do_http_response(sock);
			}
				
			//
			//do_http_response(sock);
			//bad_request(sock);
			
		}
		else
		{
			printf("other request = %s, i = %d\n", method, i);
			
			//读取http头部，不做任何处理
			do
			{
				len = get_line(sock, buf, sizeof(buf));
				printf("read line：%s \n", buf);
			}while(len > 0);
			
			unimplemented(sock);
		}
		
		
		
		
	}
	else	//出错的处理
	{
		bad_request(sock);
	}
	
}

void do_http_response(int sock)
{
	const char *main_header = "HTTP/1.0 200 OK\r\nServer: Mini Server\r\nContent-Type: text/html\r\nConnection:Close\r\n";
	const char *welcome_content = "\
	<html lang=\"zh-CN\">\n\
	<head>\n\
	<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\n\
	<title>Test</title>\n\
	</head>\n\
	<body>\n\
	<h1>This is a Test HTML!</h1>\n\
	<p>This is a Test WEB 这是一个测试网页!</p>\n\
	</body>\n\
	</html>";
	
	char send_buf[64];
	int wc_len = strlen(welcome_content);
	int len = write(sock, main_header, strlen(main_header));
	len = snprintf(send_buf, 64, "Content-Length: %d\r\n\r\n", wc_len);
	len = write(sock, send_buf, len);
	len = write(sock, welcome_content, wc_len);
}

int	 get_line(int sock, char *buf, int size)
{
	int count = 0;	//已读字节
	char ch = '\0';
	int len = 0;	//返回长度
	
	while( (count < size - 1) && ch != '\n' )
	{
		len = read(sock, &ch, 1);
		
		if(len ==1)
		{
			if (ch == '\r')
			{
				continue;
			}
			else if (ch == '\n')
			{
				buf[count] = '\0';
				break;
			}
				
			buf[count] = ch;
			count ++;
		}
		else if (len == -1)		//读取出错
		{
			perror("read failed.");
			break;
		}
		else if (len == 0)		//客户端关闭 返回0；
		{
			//perror("Client socket close.");
			fprintf(stderr, "client close.\n");
			break;
		}
	}
	
	return count;
	
}	

void bad_request(int sock)
{
	const char *reply = "HTTP/1.0 400 BAD REQUEST\r\n\
	Content-Type: text/html\r\n\
	\r\n\
	<html lang=\"zh-CN\">\n\
	<head>\n\
	<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\n\
	<title>BAD REQUEST</title>\n\
	</head>\n\
	<body>\n\
	<p>Your browser sent a bad request ! </p>\n\
	</body>\n\
	</html>\n";
	
	int len = write(sock, reply, strlen(reply));
	if(len <=0)
	{
		fprintf(stdout, "send reply failed. reason: %s\n", strerror(errno));
	}
}

void unimplemented(int sock)
{
	const char *reply = "HTTP/1.0 501 Method Not Implemented\r\n\
	Content-Type: text/html\r\n\
	\r\n\
	<html lang=\"zh-CN\">\n\
	<head>\n\
	<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\n\
	<title>Method Not Implemented</title>\n\
	</head>\n\
	<body>\n\
	<p>HTTP request method not supported. </p>\n\
	</body>\n\
	</html>\n";
	
	int len = write(sock, reply, strlen(reply));
	if(len <=0)
	{
		fprintf(stdout, "send reply failed. reason: %s\n", strerror(errno));
	}
}

void not_found(int sock)
{
	const char *reply = "HTTP/1.0 404 NOT FOUND\r\n\
	Content-Type: text/html\r\n\
	\r\n\
	<html lang=\"zh-CN\">\n\
	<head>\n\
	<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\n\
	<title>NOT FOUND</title>\n\
	</head>\n\
	<body>\n\
	<p>The server could not fulfill your request because the request... </p>\n\
	</body>\n\
	</html>\n";
	
	int len = write(sock, reply, strlen(reply));
	if(len <=0)
	{
		fprintf(stdout, "send reply failed. reason: %s\n", strerror(errno));
	}
}

void not_found_do(int sock)
{
	const char *reply = "HTTP/1.0 200 OK\r\nServer: Mini Server\r\nContent-Type: text/html\r\nConnection:Close\r\n\
	Content-Type: text/html\r\n\
	\r\n\
	<html lang=\"zh-CN\">\n\
	<head>\n\
	<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\n\
	<title>NOT FOUND</title>\n\
	</head>\n\
	<body>\n\
	<p>The server could not fulfill your request because the request... </p>\n\
	</body>\n\
	</html>\n";
	
	int len = write(sock, reply, strlen(reply));
	if(len <=0)
	{
		fprintf(stdout, "send reply failed. reason: %s\n", strerror(errno));
	}
}

void do_http_response_assign(int sock, const char *url)
{
	const char *main_header = "HTTP/1.0 200 OK\r\nServer: Mini Server\r\nContent-Type: text/html\r\nConnection:Close\r\n";
	const char *welcome_content = "\
	<html lang=\"zh-CN\">\n\
	<head>\n\
	<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\n\
	<title>Test</title>\n\
	</head>\n\
	<body>\n\
	<h1>This is a Test HTML!</h1>\n\
	<p>This is a Test WEB 这是一个测试网页!</p>\n\
	</body>\n\
	</html>";
	
	printf("-------->>>>\n");
	FILE *fp;
	fp = fopen(url, "r");	//打开文件
	if (NULL == fp)
	{
		printf(" %s Open fail !\n", url);
		return;
	}
	
	fseek(fp, 0, SEEK_END);	//设置文件指针到文件结束位置
	int filelen = ftell(fp);	//获取文件内容长度
	char* pbuf = (char *)malloc(sizeof(char)* filelen);	//申请内存空间
	if(!pbuf)
	{
		printf("Memory allocation failed ! \n");
		fclose(fp);
		return;
	}
	fseek(fp, 0, SEEK_SET);		//文件指针移动到开头位置
	fread(pbuf, filelen, 1, fp);	//读取文件内容1次读完，或者每次读1字节，读filelen次
	fclose(fp);
	if(NULL != fp)
	{
		fp = NULL;
	}
	
	
	char send_buf[64];
	//int wc_len = strlen(welcome_content);
	int len = write(sock, main_header, strlen(main_header));
	len = snprintf(send_buf, 64, "Content-Length: %d\r\n\r\n", filelen);
	len = write(sock, send_buf, len);
	len = write(sock, pbuf, filelen);
	
	free(pbuf);
	
}
