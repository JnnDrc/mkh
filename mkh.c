#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

char* strupper(char* s){
    for(char* p = s; *p; p++) *p = toupper((unsigned char)*p);
    return s;
}

char* strsanit(char* s){
    for(char* p = s; *p; p++)
        if(!isalnum((unsigned char)*p) && *p != '_') *p = '_';
    return s;
}

#define FLAG(n) (1 << n)
#define SETF(fs,f) fs |= f
#define HASF(fs,f) (fs & f)

#define F_HASIN     FLAG(0)
#define F_CONST     FLAG(1)
#define F_STATIC    FLAG(2)
#define F_DEFAULT   0

int main(int argc, char* argv[]){
    if(argc < 2){
        fprintf(stderr,"USAGE: %s input [--options]",argv[0]);
        return -1;
    }
    // parse input
    int flags = F_DEFAULT;
    char* in  = NULL;
    for(int i = 1; i < argc; i++){
        if(argv[i][0] == '-'){
            if      (!strcmp(argv[i],"--const"))  SETF(flags,F_CONST);
            else if (!strcmp(argv[i],"--static")) SETF(flags,F_STATIC);
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

    // read input ----
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
    

    // parse input name ----
    char fpath[256];
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

    if(!fname[0]){
        fprintf(stderr,"ERROR: Inalid input name");
        free(buf);
        fclose(fin);
        return -1;
    }

    // output to file ----

    // open file
    char tmp[256];
    if (ext) snprintf(tmp,sizeof(tmp),"%s_%s.h",fname,ext);
    else snprintf(tmp,sizeof(tmp),"%s.h",fname);
    FILE* fout = fopen(tmp,"w");
    if(!fout){
        fprintf(stderr,"ERROR: Failed to open file '%s'",tmp);
        free(buf);
        fclose(fin);
        return -1;
    }
    
    // make storage modifiers
    char dec_mods[64] = "";
    char def_mods[64] = "";
    if(HASF(flags,F_STATIC)){
        strcat(dec_mods,"static ");
        strcat(def_mods,"static ");
    }else strcat(dec_mods,"extern ");
    if(HASF(flags,F_CONST)){
        strcat(dec_mods,"const ");
        strcat(def_mods,"const ");
    }

    if (ext) snprintf(tmp,sizeof(tmp),"%s_%s",fname,ext);
    else     snprintf(tmp,sizeof(tmp),"%s",fname);

    strupper(tmp);
    strsanit(tmp);
    // header
    fprintf(fout,"#ifndef %s_H\n",tmp);
    fprintf(fout,"#define %s_H\n\n",tmp);
    
    fprintf(fout,"#define %s_SIZE %zu\n",tmp,insz);
    if (ext) fprintf(fout,"%sunsigned char %s_%s[%s_SIZE];\n",dec_mods,fname,ext,tmp);
    else    fprintf(fout,"%sunsigned char %s[%s_SIZE];\n",dec_mods,fname,tmp);
    fprintf(fout,"\n#endif /* %s_H */\n",tmp);
    // ----
    fprintf(fout,"\n");
    // data
    fprintf(fout,"#ifdef %s_DATA\n\n",tmp);
    if (ext) fprintf(fout,"%sunsigned char %s_%s[%s_SIZE] = {\n",def_mods,fname,ext,tmp);
    else     fprintf(fout,"%sunsigned char %s[%s_SIZE] = {\n",def_mods,fname,tmp);

    for (size_t i = 0; i < insz; i += 16) {
        fprintf(fout,"    ");
        for (size_t j = 0; j < 16; j++) {
            if (i + j < insz) fprintf(fout,"0x%02x, ", buf[i + j]);
        }
        fprintf(fout,"\n");
    }
    fprintf(fout,"};\n");
    fprintf(fout,"\n#endif /* %s_DATA */",tmp);
    // ----
    free(buf);
    fclose(fout);
    fclose(fin);
    return 0;
}
