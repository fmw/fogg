/*
fogg.c: FastCGI application that streams Ogg media resources. 

Copyright (c) 2009, Filip de Waard. All rights reserved.
This code is distributed under the terms in the LICENSE file.

*/

#define _GNU_SOURCE //enable GNU extension that includes strndup()

//TODO: read this from configuration.
#define BASE_URI "/ogg/"
char BASE_DIR[1024] = "/home/dewaard/Music/ogg/";

#include <string.h>
#include <fcgiapp.h>
#include <oggz/oggz.h>
#include <inttypes.h>

int status = 200;
char *error_msg;

struct request {
    char *path;
};

OGGZ * open_media_resource(char *path) {
    OGGZ * oggz;

    char file[1024];

    strncpy(file, BASE_DIR, strlen(BASE_DIR));
    strncat(file, path, 1024 - strlen(path) - 1);
    
    if((oggz = oggz_open(file, OGGZ_AUTO)) == NULL) {
        status = 404;
        error_msg = "Media resource wasn't found.";
    }

    return oggz;
}
struct request parse_request(char **envp) {
    //TODO: parse language preferences, et cetera.

    struct request req;
    
    char *uri = FCGX_GetParam("REQUEST_URI", envp);
    
    const int base_uri_length = strlen(BASE_URI);
    const int path_length = strlen(uri) - base_uri_length;
    
    //return 404 when there is
    //no file path to process
    if(path_length > 0) {
        status = 200;
        error_msg = "File not found";
    }
    else {
        status = 404;
        return req;
    }
    
    //FIXME: Do I have to free() this? strndup() uses malloc to
    //allocate memory (alternatively, strndupa uses alloca()).
    req.path = strndup(uri + strlen(BASE_URI), strlen(uri));

    return req;
}

int main() {
    FCGX_Stream *in, *out, *err;
    FCGX_ParamArray envp;
    
    while(FCGX_Accept(&in, &out, &err, &envp) >= 0) {
        struct request req = parse_request(envp);
        
        if(status == 200) {
            long n;
            OGGZ  * oggz = open_media_resource("epoq.ogg");
            
            //status can be modified by open_media_resource
            if(status == 200) {
                while((n = oggz_read(oggz, 1024)) > 0);

                FCGX_FPrintF(out, "Status: 200 OK\r\n");
                FCGX_FPrintF(out, "Content-type: text/html\r\n\r\n");
        
                FCGX_FPrintF(out, "<h3>%s</h3>", req.path);
                FCGX_FPrintF(out, "<p>byte offset: %" PRId64 "</p>\n", 
                    oggz_tell_units(oggz) / 1000);
                FCGX_FPrintF(out, "<p>%s</p>", BASE_URI);
            }
        }
        if(status == 404) {       
            //FIXME: use webserver 404 mechanics
            FCGX_FPrintF(out, "Status: 404\r\n");
            FCGX_FPrintF(out, "Content-type: text/html\r\n\r\n");
            FCGX_FPrintF(out, "\r\n<h3>%s</h3>", error_msg);
        }
    }
    
    return 0;
}
