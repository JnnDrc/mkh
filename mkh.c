#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

char* strupper(char* s);
char* strsanit(char* s);

char* parse_out(char* in);

void help(char* name);

#define FLAG(n)    (1 << (n))
#define SETF(fs,f) ((fs) |= (f))
#define HASF(fs,f) ((fs) & (f))

#define F_HASIN     FLAG(0)
#define F_HASOUT    FLAG(1)
#define F_CONST     FLAG(2)
#define F_STATIC    FLAG(3)
#define F_DEFAULT   0

#define MAX_PATH    256
#define MAX_SPEC    64
#define TMPSIZ      256

int main(int argc, char* argv[]){
    if(argc < 2){
        fprintf(stderr,"USAGE: %s input [--flags]",argv[0]);
        return -1;
    }
    // parse input flags -------------------------------------------------------

    // defaults ------------------------
    int flags    = F_DEFAULT;
    char* in     = NULL;
    char* out    = NULL;
    int indent   = 4;
    size_t bytes = 16;
    // parse flags ---------------------
    for(int i = 1; i < argc; i++){
        if(argv[i][0] == '-'){
            if      (!strcmp(argv[i],"--const"))  SETF(flags,F_CONST);
            else if (!strcmp(argv[i],"--static")) SETF(flags,F_STATIC);
            else if (!strcmp(argv[i],"--out") || !strcmp(argv[i],"-o")){
                if(HASF(flags,F_HASOUT)){
                    fprintf(stderr,"ERROR: Multiple output files passed, only one is supported");
                    return -1;
                }
                out = argv[++i];
                SETF(flags,F_HASOUT);
            }
            else if (!strcmp(argv[i],"--indent") || !strcmp(argv[i],"-i")) indent = atoi(argv[++i]);
            else if (!strcmp(argv[i],"--bytes")  || !strcmp(argv[i],"-b")) bytes  = atoi(argv[++i]);
            else if (!strcmp(argv[i],"--help")   || !strcmp(argv[i],"-h")){
                help(argv[0]);
                return 0;
            }
            else{
                fprintf(stderr,"ERROR: Unknown option '%s'",argv[i]);
                return -1;
            }
        }else{
            if(HASF(flags,F_HASIN)){
                fprintf(stderr,"ERROR: Multiple input files passed, only one is supported");
                return -1;
            }
            in = argv[i];
            SETF(flags, F_HASIN);
        }
    }
    if(!HASF(flags,F_HASIN)){
        fprintf(stderr,"ERROR: No input file passed");
        return -1;
    }

    // read input --------------------------------------------------------------

    FILE* fin = fopen(in,"rb");
    if(!fin){
        fprintf(stderr,"ERROR: Failed to open file '%s'",in);
        return -1;
    }

    if(fseek(fin,0L,SEEK_END) != 0){
        fprintf(stderr,"ERROR: Failed to seek file");
        return -1;
    }

    long sz = ftell(fin);
    if(sz < 0){
        fprintf(stderr,"ERROR: Failed to tell file size");
        fclose(fin);
        return -1;
    }
    if(sz == 0){
        fprintf(stderr,"ERROR: File is empty");
        fclose(fin);
        return -1;
    }
    rewind(fin);
    size_t insz = (size_t)sz;

    unsigned char* buf = malloc(insz);
    if(!buf){
        fprintf(stderr,"ERROR: Failed to allocate memory");
        fclose(fin);
        return -1;
    }
    if (fread(buf,1,insz,fin) != insz){
        fprintf(stderr,"ERROR: Failed to read input file");
        free(buf);
        fclose(fin);
        return -1;
    }
    fclose(fin);

    // parse output name -------------------------------------------------------

    if(!HASF(flags,F_HASOUT)) out = parse_out(in);
    if(!out){
        fprintf(stderr,"ERROR: Failed to allocate memory");
        free(buf);
        return -1;
    }
    if(!out[0]){
        fprintf(stderr,"ERROR: Invalid input name");
        free(buf);
        return -1;
    }

    // output to file ----------------------------------------------------------

    // open file -----------------------
    char tmp[TMPSIZ];
    snprintf(tmp,sizeof(tmp),"%s.h",out);
    FILE* fout = fopen(tmp,"w");
    if(!fout){
        fprintf(stderr,"ERROR: Failed to open file '%s'",tmp);
        free(buf);
        return -1;
    }
    
    // make storage modifiers ----------
    char dec_mods[MAX_SPEC] = "";
    char def_mods[MAX_SPEC] = "";
    if(HASF(flags,F_STATIC)){
        strcat(dec_mods,"static ");
        strcat(def_mods,"static ");
    }else strcat(dec_mods,"extern ");
    if(HASF(flags,F_CONST)){
        strcat(dec_mods,"const ");
        strcat(def_mods,"const ");
    }
    
    snprintf(tmp,sizeof(tmp),"%s",out);
    strupper(tmp);
    strsanit(tmp);

    // header --------------------------
    fprintf(fout,"#ifndef %s_H\n",tmp);
    fprintf(fout,"#define %s_H\n\n",tmp);
    
    fprintf(fout,"#define %s_SIZE %zu\n",tmp,insz);
    fprintf(fout,"%sunsigned char %s[%s_SIZE];\n",dec_mods,out,tmp);
    fprintf(fout,"\n#endif /* %s_H */\n",tmp);
    // -------------
    fprintf(fout,"\n");
    // data ----------------------------
    fprintf(fout,"#ifdef %s_DATA\n\n",tmp);
    fprintf(fout,"%sunsigned char %s[%s_SIZE] = {\n",def_mods,out,tmp);

    for (size_t i = 0; i < insz; i += bytes) {
        for(int i = 0; i < indent; i++) fprintf(fout," ");
        for (size_t j = 0; j < bytes; j++) {
            if (i + j < insz) fprintf(fout,"0x%02x, ", buf[i + j]);
        }
        fprintf(fout,"\n");
    }
    fprintf(fout,"};\n");
    fprintf(fout,"\n#endif /* %s_DATA */",tmp);

    // exit --------------------------------------------------------------------
    free(buf);
    fclose(fout);
    return 0;
}

char* strupper(char* s){
    for(char* p = s; *p; p++) *p = toupper((unsigned char)*p);
    return s;
}

char* strsanit(char* s){
    for(char* p = s; *p; p++)
        if(!isalnum((unsigned char)*p) && *p != '_') *p = '_';
    return s;
}

char* parse_out(char* in){
    char fpath[MAX_PATH];
    char* fname = fpath;

    // get name only
    snprintf(fpath,sizeof(fpath),"%s",in);
    char* d1 = strrchr(fpath,'/');
    char* d2 = strrchr(fpath,'\\');
    char* dir = d1 > d2 ? d1 : d2;
    if(dir) fname = dir+1;
    // get extension
    char* dot = strrchr(fname,'.');
    char* ext = NULL;
    if(dot && dot != fname){
        *dot = '\0';
        ext  = dot + 1;
    }
    // sanitize
    strsanit(fname);
    if(ext) strsanit(ext);

    if(!isalpha((unsigned char)fname[0]) && fname[0] != '_') fname[0] = '_';
    // concat name and extension (if have)
    char* out = NULL;
    if(ext){
        size_t flen = strlen(fname);
        size_t elen = strlen(ext);
        out = malloc(flen + elen + 2);
        if(!out) return NULL;
        snprintf(out,flen + elen + 2, "%s_%s",fname,ext);
    }else{
        size_t flen = strlen(fname);
        out = malloc(flen + 1);
        if(!out) return NULL;
        snprintf(out,flen,"%s",fname);
    }
    return out;
}

void help(char* name){
    fprintf(stdout,"USAGE: %s input [--flags]\n",name);
    fprintf(stdout,"FLAGS: \n");
    fprintf(stdout,"    --out     -o  set output to <name>.h       [default=input.h]\n");
    fprintf(stdout,"    --const       make data constant           [default=false]\n");
    fprintf(stdout,"    --static      make data static             [default=false]\n");
    fprintf(stdout,"    --indent  -i  set indentation size         [default=4]\n");
    fprintf(stdout,"    --bytes   -b  set amount of bytes per line [default=16]");
    fprintf(stdout,"    --help    -h  print this help\n");
}
