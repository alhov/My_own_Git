#include "misc.h"
#include "fsop.h"
#include "hashFunc.h"
#include "workTree.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void setMode(int mode, char * path) {
    char buff[100];
    sprintf(buff, "chmod %d %s", mode, path);
    system(buff);
}

WorkFile* createWorkFile(char* name) {
    WorkFile* new = (WorkFile*)malloc(sizeof(WorkFile));
    new->name = strdup(name);
    new->hash = NULL;
    new->mode = 0;
    return new;
}

WorkFile* createWorkFile2(char* name, char* hash, int mode) {
    WorkFile* new = (WorkFile*)malloc(sizeof(WorkFile));
    new->name = strdup(name);
    new->hash = hash == NULL ? NULL : strdup(hash);
    new->mode = mode;
    return new;
}

/**
 * @brief hash should not be NULL
 * 
 * @param wf 
 * @return char* 
 */
char* wfts(WorkFile* wf) {
    assert(wf->name != NULL); assert(wf->hash != NULL);
    char buf[MAXL];
    sprintf(buf,"%s\t%s\t%d", wf->name, wf->hash, wf->mode);
    return strdup(buf);
}

WorkFile* stwf(char* ch) {
    char buf1[MAXL], buf2[MAXL]; int buf3;
    sscanf(ch, "%s\t%s\t%d", buf1, buf2, &buf3);
    return createWorkFile2(buf1, buf2, buf3);
}

void freeWf(WorkFile* wf) {
    free(wf->name); // free(NULL), nothing happens
    free(wf->hash);
    free(wf);
}

WorkTree* initWorkTree() {
    WorkTree* wt = (WorkTree*)malloc(sizeof(WorkTree));
    wt->size = WTS;
    wt->tab = (WorkFile*)malloc(sizeof(WorkFile)*(szt)(wt->size));
    wt->n = 0;
    return wt;
}

/**
 * @brief Return the position of file of folder named name in wt, -1 if not found
 */
int inWorkTree(WorkTree* wt, char* name) {
    for(int i=0; i<wt->n; ++i) {
        if(strcmp(wt->tab[i].name, name) == 0)
            return i;
    }
    return -1;
}

int appendWorkTree(WorkTree* wt, char* name, char* hash, int mode) {
    if(wt->size == wt->n) {
        err("WorkTree full error\n");
        return -1;
    }
    if(inWorkTree(wt, name) != -1) return 0;
    // protestation of not having double pointers
    WorkFile *w = createWorkFile2(name, hash, mode);
    wt->tab[(wt->n)++] = *w; free(w);
    return 0;
}

char* wtts(WorkTree* wt) {
    char buf[MAXL];
    char* p = buf;
    for(int i=0; i<wt->n; ++i) {
        char *s = wfts(&(wt->tab[i]));
        sprintf(p, "%s\n", s);
        p += strlen(p);
        free(s);
    }
    return strdup(buf);
}

WorkTree* stwt(const char* s) {
    WorkTree* wt = initWorkTree();
    char buf[MAXL];
    while(*s) {
        sscanf(s, "%[^\n]", buf);
        WorkFile *wf = stwf(buf);
        assert(appendWorkTree(wt, wf->name, wf->hash, wf->mode) == 0);
        freeWf(wf);
        s += strlen(buf)+1;
    }
    return wt;
}

int wttf(WorkTree* wt, char* file) {
    FILE* f = fopen(file, "w");
    assert(f);
    char* s = wtts(wt);
    fprintf(f, "%s", s);
    free(s);
    fclose(f);
    return 0;
}

WorkTree* ftwt(const char* file) {
    FILE* f = fopen(file, "r");
    assert(f);
    char buf[MAXL];
    int m = (int)fread(buf, MAXL-1, sizeof(char), f);
    buf[m] = 0;
    fclose(f);
    return stwt(buf);
}

void freeWt(WorkTree* wt) {
    for(int i=0;i<wt->n;++i) {
        free(wt->tab[i].name);
        free(wt->tab[i].hash);
    }
    free(wt->tab);
    free(wt);
}

/**
 * @brief Take a snapshot of WorkTree wt with extension .t
 * @return char* the hash of the string representation of wt
 */
char* blobWorkTree(WorkTree* wt) {
    char* s = wtts(wt);
    char* hash = blobStringExt(s, ".t");
    free(s);
    return hash;
}

WorkTree* getWtFromPath(char* path) {
    WorkTree* newWt = initWorkTree();
    List* l = listdir(path);
    for(Cell* c = *l; c; c = c->next) {
        // err("f: %s\n", c->data);
        appendWorkTree(newWt, c->data, NULL, 0);
    }
    freeList(l);
    return newWt;
}

/**
 * @brief Create a snapshot of the files and folders of wt at path
 * and fill in the hash and mode entry
 */
char* saveWorkTree(WorkTree* wt, char* path) {
    for(int i=0;i<wt->n;++i) {
        char buf[MAXL];
        strcpy(buf, path);
        append(buf, "/");
        append(buf, wt->tab[i].name);
        if(isDir(buf)) {
            WorkTree* newWt = getWtFromPath(buf);
            wt->tab[i].hash = saveWorkTree(newWt, buf);
            wt->tab[i].mode = getChmod(buf);
            freeWt(newWt);
        } else {
            blobFile(buf);
            wt->tab[i].hash = sha256file(buf);
            wt->tab[i].mode = getChmod(buf);
        }
    }
    return blobWorkTree(wt);
}