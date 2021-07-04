#include <stdio.h>
#include <string.h>

#define NAME_LEN 256
#define LINE_LEN 256

#define REPLACE_NUM 2
const char *FIND[REPLACE_NUM] = {"\n", "\""};
const char *REPLACE[REPLACE_NUM] = {"\\\n", "\\\""};

//var_name will be replaced by file name
char fileStart[LINE_LEN] = "const char* var_name = \"\\\n";
char* fileEnd = "\";\n";
char * newFileNameEnd = ".h";


void replaceN(char* line,const char* orig,const char* new, int times){
	char* buf;
	if(times==0) return; //sem tempo irmao
	
	if((times==-1||--times>0) && (buf = strstr(line,orig))!=NULL){ //find orig
		for(const char *c=orig;*c;c++) buf++; //advance buf
		replaceN(buf,orig,new,times); //repeat until the last occurrence
	}
	//this will run first for the last match
	if((buf = strstr(line,orig))!=NULL){ 
		char tmp[LINE_LEN];
		int i = buf-line; //pointer difference
		strncpy(tmp,line,i); //copy everything before the match
		for(const char *k=orig;*k;k++) buf++; //buf++; //skip find string
		for(const char *k=new;*k;k++) tmp[i++]=*k; //copy replace chars
		for(;*buf;buf++) tmp[i++]=*buf; //copy the rest of the string

		tmp[i]='\0';
		strcpy(line,tmp);		
	}
}
inline void replace(char* line,const char* orig,const char* new){replaceN(line, orig, new, 1);}
inline void replaceAll(char* line,const char* orig,const char* new){replaceN(line,orig,new,-1);}

void cleanFileName(char* text){
	replaceAll(text,".html","");
	replaceAll(text,".htm","");
	replaceAll(text,".","");
	replaceAll(text,"/","");
	replaceAll(text,"\\","");
}

int main (int argc, char **argv){
	char outName[NAME_LEN];
	char line[LINE_LEN];
	FILE *inFile, *outFile;
	inFile = fopen(argv[1],"r");
	strcpy(outName,argv[1]);
	strcat(outName,newFileNameEnd);
	outFile = fopen(outName,"w");
	char inName[NAME_LEN];
	strcpy(inName,argv[1]);
	
	//replace 'var_name' for filename without extension, then add fileStart
	cleanFileName(inName);
	replaceAll(fileStart,"var_name",inName);
	fputs(fileStart,outFile);
	
	while(fgets(line,LINE_LEN,inFile)!=NULL){
		for(int i =0;i<REPLACE_NUM;i++)
			replaceAll(line,FIND[i],REPLACE[i]);

		fputs(line,outFile);
				
	}
	fputs(fileEnd,outFile);
	fclose(inFile);
	fclose(outFile);
	
	
}