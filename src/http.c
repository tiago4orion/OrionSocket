/*
   OrionSocket - Socket Header
   --------------------------------

   Author: Tiago Natel de Moura <tiago4orion@gmail.com>

   Copyright 2007, 2008 by Tiago Natel de Moura. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

 */
#include "http.h"
#include "err.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// Berkeley Sockets
#include <netdb.h>
#include <sys/socket.h>

void orion_initCookie(orion_cookie** cookie2)
{
	orion_cookie* cookie = (orion_cookie *) malloc(sizeof(orion_cookie));

	if (!cookie)
	{
#ifdef ORIONSOCKET_DEBUG
		fprintf(stderr, "[ERROR] Erro ao alocar memória.\n");
#endif
		return;
	}

	cookie->name = NULL;
	cookie->value = NULL;
	cookie->domain = NULL;
	cookie->path = NULL;
	cookie->expires = NULL;
	cookie->proto = NULL;

	*cookie2 = cookie;

}

void orion_setCookie(	orion_cookie* cookie, 
						const char* name, 
						const char* value, 
						const char* domain, 
						const char* path, 
						const char* proto, 
						const char* expires)
{	
	if (!strlen(name))
		return;

	cookie->name = strdup(name);
	cookie->value = strdup(value && strlen(value) ? value : "");
	cookie->domain = strdup(domain && strlen(domain) ? domain : "");
	cookie->path = strdup(path && strlen(path) ? path : "");
	cookie->proto = strdup(proto && strlen(proto) ? proto : "");
	cookie->expires = strdup(expires && strlen(expires) ? expires : "");
}

_uint8 orion_addCookie(orion_httpResponse *res, orion_cookie* cookie)
{
	_uint8 len = res->cookieLen;
	res->cookie = (orion_cookie*) orion_realloc(res->cookie, sizeof(orion_cookie)*(len+1));	

	if (!res->cookie)
	{
#ifdef ORIONSOCKET_DEBUG
		fprintf(stderr, "[ERROR] Erro ao alocar memoria.\n");
#endif
		return ORIONSOCKET_ERR_ALLOC;
	}

	res->cookie[len] = *cookie;
	res->cookieLen++;

	return ORIONSOCKET_OK;
}

void orion_cleanupCookie(orion_cookie* cookie)
{
	if (cookie)
	{
		ORIONFREE(cookie->name);
		ORIONFREE(cookie->value);
		ORIONFREE(cookie->domain);
		ORIONFREE(cookie->path);
		ORIONFREE(cookie->expires);
		ORIONFREE(cookie->proto);
	}

	ORIONFREE(cookie);
}

void orion_initHttpRequest(orion_httpRequest **req2)
{
  orion_httpRequest *req = NULL;
    
  req = (orion_httpRequest *) malloc(sizeof(orion_httpRequest));
  if (!req)
  {
#ifdef ORIONSOCKET_DEBUG
    fprintf(stderr, "[ORION][ERROR] Não foi possivel alocar memória.\n"); 
#endif
	*req2 = NULL;
    return;
  }

  req->host = NULL;
  req->port = 80;                 /* Default port         */
  req->path = NULL;
  req->method = ORION_METHOD_GET;       /* Default HTTP Method  */
  req->file_ext = NULL;
  req->header = NULL;
  req->headerLen = 0;
  req->cookie = NULL;
  req->cookieLen = 0;
  req->query = NULL;
  req->option = 0x0;             /* No option */
    
  *req2 = req;
}

void orion_cleanupHttpRequest(orion_httpRequest *req)
{
    int i;
    
    if (req)
    {
        ORIONFREE(req->host);
        req->port = 80;
        ORIONFREE(req->path);
        ORIONFREE(req->file_ext);

        for (i = 0; i < req->headerLen; i++)
        {
            ORIONFREE(req->header[i].name);
            ORIONFREE(req->header[i].value);
        }

        if (req->headerLen > 0)
            ORIONFREE(req->header);
        req->headerLen = 0;

        for (i = 0; i < req->cookieLen; i++)
        {
            orion_cleanupCookie(&req->cookie[i]);
        }

        if (req->cookieLen > 0)
            ORIONFREE(req->cookie);
        req->cookieLen = 0;

        ORIONFREE(req);
    }
}

void orion_setHttpRequestHost(orion_httpRequest *req, const char* host, _uint16 port)
{
    req->host = strdup(host);
    req->port = port;
}

void orion_setHttpRequestPath(orion_httpRequest *req, const char* path)
{
    ORIONFREE(req->path);
    req->path = strdup(path);
}

//
// Adiciona um Header à requisição HTTP da estrutura httpRequest
// @param orionHttpRequest*   req
// @param const char* name
// @param const char* value
_uint8 orion_setHttpRequestHeader(orion_httpRequest *req, const char* name, const char* value)
{
	_uint8 len = 0, lenName = 0, lenValue = 0;
	len = req->headerLen;
	
    lenName = strlen(name);
    lenValue = strlen(value);
    
	req->header = (nameValue *) orion_realloc(req->header, sizeof(nameValue)*(len+1));
	if (!req->header)
	{
#ifdef ORIONSOCKET_DEBUG
		fprintf(stderr, "[ERROR] Erro ao alocar memória.\n");
#endif
		return ORIONSOCKET_ERR_ALLOC;
	}
    
	req->header[len].name = (char *) malloc(sizeof(char) * lenName + 1);
	req->header[len].value = (char *) malloc(sizeof(char) * lenValue + 1);
	if (!req->header[len].name || !req->header[len].value)
	{
#ifdef ORIONSOCKET_DEBUG
		fprintf(stderr, "[ERROR] Erro ao alocar memória.\n");
#endif
		return ORIONSOCKET_ERR_ALLOC;
	}
	
	strncpy(req->header[len].name, name, lenName);
	strncpy(req->header[len].value, value, lenValue);
	req->header[len].name[lenName] = '\0';
	req->header[len].value[lenValue] = '\0';
	
    
	req->headerLen++;
    
	return ORIONSOCKET_OK;    
}

void orion_setHttpRequestOption(orion_httpRequest* req, _uint16 option)
{
    req->option |= option;
}

// Estabelece uma conexão única no host passado em httpRequest *
// Retorna o conteudo da página.
// 
_uint8 orion_httpRequestPerform(orion_httpRequest *req, char** response)
{
	int sockfd, n = 0, lengthBuffer = 0;
	char reqBuffer[ORION_HTTP_REQUEST_MAXLENGTH], temp[ORION_HTTP_BIG_RESPONSE];
	char *localBuffer = NULL;
    
	memset(reqBuffer, '\0', sizeof(char) * ORION_HTTP_REQUEST_MAXLENGTH);
	memset(temp, '\0', sizeof(char) * ORION_HTTP_RESPONSE_LENGTH);
    
	orion_assembleHttpRequest(req, reqBuffer);
    
	DEBUG_HTTPREQUEST(req);

    if ((req->option & ORION_OPTDEBUG_REQUEST)==ORION_OPTDEBUG_REQUEST)
        printf("%s\n", reqBuffer);
    
	sockfd = orion_tcpConnect(req->host, req->port);
    
	if (sockfd < 0)
	{
#ifdef ORIONSOCKET_DEBUG
		perror("[ERROR] Erro ao conectar.\n");
#endif
		close(sockfd);
		return errno;
	}
    
	if (send(sockfd, reqBuffer, strlen(reqBuffer), 0) < 0)
	{
#ifdef ORIONSOCKET_DEBUG
		perror("[ERROR] Erro ao enviar requisição.\n");
#endif
		close(sockfd);
		return errno;
	}
    
	localBuffer = (char *) malloc(sizeof(char) * 1);
	localBuffer[0] = '\0';

	while((n = read(sockfd, temp, ORION_HTTP_BIG_RESPONSE)) > 0)
	{
        lengthBuffer = n + strlen(localBuffer) + 1;
		localBuffer = (char *) orion_realloc(localBuffer, lengthBuffer);
		temp[n-1] = '\0';
		strncat(localBuffer, temp, lengthBuffer-1);
		bzero(temp, ORION_HTTP_BIG_RESPONSE);
	}
    
	close(sockfd);
    
	*response = localBuffer;
    
	return ORIONSOCKET_OK;
}

_uint8 orion_httpReqRes(orion_httpRequest* req, orion_httpResponse** res2)
{
    int status = ORIONSOCKET_OK;
    char* response = NULL;
    orion_httpResponse* res = NULL;
    
    status = orion_httpRequestPerform(req, &response);
    
    if (status == ORIONSOCKET_OK)
    {
        orion_initHttpResponse(&res);
        orion_assembleHttpResponse(res, response);
        
        *res2 = res;
        
        ORIONFREE(response);
    }
    
    return status;
}

_uint8 orion_httpGet(orion_httpRequest* req, void (* callback)(char*,_uint32), _uint32 count)
{
    int sockfd, n;
    char reqBuffer[ORION_HTTP_REQUEST_MAXLENGTH], responseBuffer[count];
    
    memset(reqBuffer, 0, ORION_HTTP_REQUEST_MAXLENGTH);
    memset(responseBuffer, 0, count);
    
    orion_assembleHttpRequest(req, reqBuffer);
    
    DEBUG_HTTPREQUEST(req);

    if ((req->option & ORION_OPTDEBUG_REQUEST)==ORION_OPTDEBUG_REQUEST)
        printf("%s\n", reqBuffer);

    sockfd = orion_tcpConnect(req->host, req->port);
    if (sockfd < 0)
        return errno;
    
    if (write(sockfd, reqBuffer, strlen(reqBuffer)) < 0)
    {
#ifdef ORIONSOCKET_DEBUG
		perror("[ERROR] Erro ao enviar requisição.\n");
#endif
		close(sockfd);
		return errno;
	}
	
	n = read(sockfd, responseBuffer, count-1);

	while (n > 0)
	{
	    responseBuffer[n] = '\0';
	    (*callback)(responseBuffer, n);
	    memset(responseBuffer, 0, count);
	    n = read(sockfd, responseBuffer, count-1);
	}
	
	close(sockfd);
	
	return ORIONSOCKET_OK;
}

// Monta a Requisição HTTP a partir da estrutura httpRequest
// @param orionHttpRequest*      req
// @param char *            reqBuffer
void orion_assembleHttpRequest(orion_httpRequest* req, char* reqBuffer)
{
	_uint32 size = 0, i;
	char temp[10];
	bzero(temp, 10);
    
    // It is safe.
	strcpy(reqBuffer, orion_getStrMethod(req->method));
	strncat(reqBuffer, " ", ORION_HTTP_REQUEST_MAXLENGTH-1);
	if (!req->path)
	    req->path = strdup("/");
	
	strncat(reqBuffer, req->path, ORION_HTTP_REQUEST_MAXLENGTH-1);
	strncat(reqBuffer, " ", ORION_HTTP_REQUEST_MAXLENGTH-1);
	strncat(reqBuffer, ORION_HTTP_PROTOCOL, ORION_HTTP_REQUEST_MAXLENGTH-1);
	strncat(reqBuffer, "\n", ORION_HTTP_REQUEST_MAXLENGTH-1);    
    
    _uint8 _host_header_exists = 0;

    for (i = 0; i < req->headerLen; i++)
        if (strcasecmp(req->header[i].name, "Host") == 0)
            _host_header_exists = 1;

    if (!_host_header_exists)
    {
        strncat(reqBuffer, "Host: ", ORION_HTTP_REQUEST_MAXLENGTH-1);
        strncat(reqBuffer, req->host, ORION_HTTP_REQUEST_MAXLENGTH-1);
        strncat(reqBuffer, "\n", ORION_HTTP_REQUEST_MAXLENGTH);        
    }
    
	for (i = 0; i < req->headerLen; i++)
	{
		strncat(reqBuffer, req->header[i].name, ORION_HTTP_REQUEST_MAXLENGTH-1);
		strncat(reqBuffer, ": ", ORION_HTTP_REQUEST_MAXLENGTH-1);
		strncat(reqBuffer, req->header[i].value, ORION_HTTP_REQUEST_MAXLENGTH-1);
		strncat(reqBuffer, "\n", ORION_HTTP_REQUEST_MAXLENGTH-1);
	}

	if (req->method == ORION_METHOD_POST)
	{
		strncat(reqBuffer, "Content-Length: ", ORION_HTTP_REQUEST_MAXLENGTH-1);
		
		size += strlen(req->query);     

		sprintf(temp, "%d\n", size);
		strncat(reqBuffer, temp, ORION_HTTP_REQUEST_MAXLENGTH-1);

		strncat(reqBuffer, req->query, ORION_HTTP_REQUEST_MAXLENGTH-1);
	}
 
	strncat(reqBuffer, "\n", ORION_HTTP_REQUEST_MAXLENGTH-1);
}

void orion_initHttpResponse(orion_httpResponse** res2)
{
    orion_httpResponse* res = NULL;
    
    res = (orion_httpResponse*) malloc(sizeof(orion_httpResponse));
    res->version = 0;
    res->code = 0;
    res->message = NULL;
    res->serverName = NULL;
    res->date = NULL;
    res->expires = NULL;
    res->location = NULL;
    res->mime_version = NULL;
    res->content_type = NULL;
    res->content_length = 0;
    res->charset = NULL;
    res->header = NULL;
    res->headerLen = 0;
    res->cookie = NULL;
    res->cookieLen = 0;
    
    res->body = NULL;
    
    *res2 = res;    
}

void orion_cleanupHttpResponse(orion_httpResponse* res)
{
    _uint8 i;
    
    if (res)
    {
        res->version = 0;
        res->code = 0;
        ORIONFREE(res->message);
        res->serverName = NULL;
        res->date = NULL;
        res->expires = NULL;
        res->location = NULL;
        res->content_type = NULL;
        res->content_length = 0;
        ORIONFREE(res->charset);
        res->mime_version = NULL;
        
        for (i = 0; i < res->headerLen; i++)
        {
            ORIONFREE(res->header[i].name);
            ORIONFREE(res->header[i].value);
        }
        
        if (res->headerLen > 0)
        {
            ORIONFREE(res->header);
            res->headerLen = 0;
        }

        for (i = 0; i < res->cookieLen; i++)
        {
			orion_cleanupCookie(&res->cookie[i]);
        }

        res->cookieLen = 0;
        ORIONFREE(res->cookie);        
        ORIONFREE(res->body);        
        ORIONFREE(res);
    }
}

void orion_setHttpResponseHeader(orion_httpResponse* res, const char* name, const char* value)
{
    res->headerLen++;
    res->header = (nameValue *) orion_realloc(res->header, sizeof(nameValue)*res->headerLen);
    res->header[res->headerLen-1].name = strdup(name);
    res->header[res->headerLen-1].value = strdup(value);
}

void orion_assembleCookie(orion_cookie* cookie, char* line)
{
	char* lineBuffer = strdup(line);
	char* bufHandle = lineBuffer;
	int i, sz = strlen(lineBuffer);

	for (i = 0; i < sz && bufHandle[i] != '='; i++);

	if (i == sz-1)
		return;

	bufHandle[i] = '\0';

	cookie->name = strdup(bufHandle);
	bufHandle += i+1;

	sz = strlen(bufHandle);
	for (i = 0; i < sz && bufHandle[i] != ';'; i++);

	if (i == sz-1) 
	{
	    free(lineBuffer);
		return;
	}

	bufHandle[i] = '\0';

	cookie->value = strdup(bufHandle);
	bufHandle += i + 2;

	while (1)
	{
		if (!strncasecmp("path", bufHandle, 4))
		{
			ORION_GETPARTCOOKIE(path,4);
			
		} else if (!strncasecmp("expires", bufHandle, 7))
		{
			ORION_GETPARTCOOKIE(expires,7);
		} else if (!strncasecmp("domain", bufHandle, 6))
		{
			ORION_GETPARTCOOKIE(domain,6);
		} else 
			break;
	}

	sz = strlen(bufHandle);

	if (sz > 0)
	{
		for (i = 0; i < sz && bufHandle[i] != ';'; i++);

		if (i > 0)
		{
			bufHandle[i] = '\0';
			cookie->proto = strdup(bufHandle);
		}
	}
	
	free(lineBuffer);	
}

void orion_parseResponseLine(orion_httpResponse *res, char* line)
{
    char* bufHandle = NULL;
    char* name = NULL;
    char* value = NULL;
    
    if (!strncmp(line, "HTTP/1.", 7))
    {
        if (!strncmp(line, "HTTP/1.0", 8))
        {
            res->version = ORION_HTTP_PROTOCOL_1_0;
        } else if (!strncmp(line, "HTTP/1.1", 8))
        {
            res->version = ORION_HTTP_PROTOCOL_1_1;
        } else {
            fprintf(stderr, "[error] unknown protocol: %s\n", line);
            res->version = ORION_HTTP_PROTOCOL_UNKNOWN;
        }
                
        bufHandle = line + 9;
        bufHandle[3] = '\0';
        
        // code = XXX
        res->code = atoi(bufHandle);
                
        bufHandle = bufHandle+4;
        
        // message is the rest  of response
        res->message = strdup(bufHandle);
    } else
    if (!strncmp(line, "Server: ", 8))
    {
        bufHandle = line + 8;
        
        orion_setHttpResponseHeader(res, "Server", bufHandle);
        res->serverName = res->header[res->headerLen-1].value;
    } else
    if (!strncmp(line, "Date: ", 6))
    {
        bufHandle = line + 6;
        
        orion_setHttpResponseHeader(res, "Date", bufHandle);
        res->date = res->header[res->headerLen-1].value;
    } else if (!strncmp(line, "Expires: ", 9))
    {
        bufHandle = line + 9;
        orion_setHttpResponseHeader(res, "Expires", bufHandle);
        res->expires = res->header[res->headerLen-1].value;
    } else
    if (!strncmp(line, "Location: ", 10))
    {
        bufHandle = line + 10;
        value = bufHandle;
        orion_setHttpResponseHeader(res, "Location", value);
        res->location = res->header[res->headerLen-1].value;
    } else
    if (!strncmp(line, "Mime-Version: ", 14))
    {
        bufHandle = line + 14;
        orion_setHttpResponseHeader(res, "Mime-Version", bufHandle);
        res->mime_version = res->header[res->headerLen-1].value;
    } else 
    if (!strncmp(line, "Content-Type: ", 14))
    {    
        bufHandle = line + 14;
        orion_setHttpResponseHeader(res, "Content-Type", bufHandle);
        res->content_type = res->header[res->headerLen-1].value;
    } else 
    if (!strncmp(line, "Content-Length: ", 16))
    {
        bufHandle = line + 16;
        orion_setHttpResponseHeader(res, "Content-Length", bufHandle);
        res->content_length = atoi(res->header[res->headerLen-1].value);
    } else
    if (!strncmp(line, "Set-Cookie: ", 12))
    {
        bufHandle = line + 12;
		
        orion_cookie *c = NULL;
        orion_initCookie(&c);
        orion_assembleCookie(c, bufHandle);
        orion_addCookie(res, c);
    } else {
        bufHandle = line;
        int pos = orion_linearSearchChar(bufHandle, ':');
        if (pos > 0)
        {        
            bufHandle[pos] = '\0';
            name = bufHandle;
            bufHandle = bufHandle+pos+2; // Name: value.  ** jumping the space **
            value = bufHandle;
            
            orion_setHttpResponseHeader(res, name, value);
        }
    }
}

void orion_assembleHttpResponse(orion_httpResponse* res, char* buf)
{
    char* bufHandle = NULL, *line = NULL;
    int pos_endl = 0;
    
    bufHandle = buf;
    
    // pegando somente o header
    for (pos_endl = 0; pos_endl < strlen(bufHandle); pos_endl++)
    {
        if (    (bufHandle[pos_endl] == '\n' && bufHandle[pos_endl+1] == '\n') ||
                (bufHandle[pos_endl] == '\r' && bufHandle[pos_endl+1] == '\n' && bufHandle[pos_endl+2] == '\r' && bufHandle[pos_endl+3]=='\n')  ||
                (bufHandle[pos_endl] == '\r' && bufHandle[pos_endl+1] == '\n' && bufHandle[pos_endl+2] == '\r')
        )  break; 
    }
    bufHandle[pos_endl] = '\0';
    
    while ((line = orion_getNextLine(bufHandle)))
    {
        bufHandle = bufHandle + strlen(line) + 1;
        orion_parseResponseLine(res, line);
    }
    
    res->body = strdup(buf+pos_endl+2);
}



