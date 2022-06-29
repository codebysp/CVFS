#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

using namespace std;

// Macro define
#define MAXINODE 50

#define READ 1
#define WRITE 2

#define MAXFILESIZE 2048

#define REGULAR 1
#define SPECIAL 2

#define START 0
#define CURRENT 1
#define END 2

// Structure Declaration
typedef struct superblock
{
    int TotalInodes;
    int FreeInode;
} SUPERBLOCK, *PSUPERBLOCK;

typedef struct inode
{
    char FileName[50];
    int InodeNumber;
    int FileSize; // allocatted size for file
    int FileActualSize; // actual file size
    int FileType; // regular or special 
    char *Buffer; // block of data in file
    int LinkCount; // there is one link count in whole project if we create to shortcut of file the it will increase
    int ReferenceCount; // how many References are refering to that file means how many are accesing file at acurent time
    int permission; // 1 23
    struct inode *next;// point to next node in linked list of inode
} INODE, *PINODE, **PPINODE;

typedef struct filetable
{
    int readoffset;
    int writeoffset;
    int count;
    int mode; // 1    2   3
    PINODE ptrinode;
} FILETABLE, *PFILETABLE;

typedef struct ufdt
{
    PFILETABLE ptrfiletable;
} UFDT;

// Global Variable Declaration
UFDT UFDTArr[50];
SUPERBLOCK SUPERBLOCKobj;
PINODE head = NULL;

// Man Function
void man(char *name)
{
    if (name == NULL)
    {
        return;
    }

    if (strcmp(name, "create") == 0)
    {
        printf("Description: used to create new regular file\n");
        printf("Usage: create File_name Permission\n");
    }
    else if (strcmp(name, "read") == 0)
    {
        printf("Description:Used to read data from regular file\n");
        printf("Usage: read File_name No_Of_Bytes_to_Read\n");
    }
    else if (strcmp(name, "write") == 0)
    {
        printf("Description: used to write into regular file\n");
        printf("Usage: write File_name\n After this enter the data that we want to write\n");
    }
    else if (strcmp(name, "ls") == 0)
    {
        printf("Description: used to list all information of files\n");
        printf("Usage:ls\n");
    }
    else if (strcmp(name, "stat") == 0)
    {
        printf("Desription : used to display information of file\n");
        printf("Usage: stat File_name\n");
    }
    else if (strcmp(name, "fstat") == 0)
    {
        printf("Description: Used to Display information of file\n");
        printf("Usage: stat File_Descriptor\n");
    }
    else if (strcmp(name, "truncate") == 0)
    {
        printf("Description: used to remove data from file\n");
        printf("Usage: truncate File_name\n");
    }
    else if (strcmp(name, "open") == 0)
    {
        printf("Description: used to open file\n");
        printf("Usage: open File_name mode\n");
    }
    else if (strcmp(name, "close") == 0)
    {
        printf("Description: used to close opened file\n");
        printf("Usage: close File_name\n");
    }
    else if (strcmp(name, "closeall") == 0)
    {
        printf("Description: used to close all opened file\n");
        printf("Usage: close File_name\n");
    }
    else if (strcmp(name, "lseek") == 0)
    {
        printf("Description:used to change the offset\n");
        printf("Usage: lseek File_Name changeInOffset StartPoint\n");
    }
    else if (strcmp(name, "rm") == 0)
    {
        printf("Description:Used to delete the file\n");
        printf("Usage: rm File_Name\n");
    }
    else
    {
        printf("ERROR: No manual entry available\n");
    }
}

void DisplayHelp()
{
    printf("ls : To list out all files\n");
    printf("clear: To clear Console\n");
    printf("open : To open the file\n");
    printf("close : To close the file\n");
    printf("closeall : To close all opened files\n");
    printf("read : To read contents from file\n");
    printf("write : To write contents into file\n");
    printf("exits : To Terminate file System\n");
    printf("stat : To Display information of file using file descriptor\n");
    printf("truncate : To Remove all data from file\n");
    printf("rm : To delet the file\n");
}

int GetFDFromName(char *name)
{
    int i = 0;

    while (i < MAXINODE)
    {
        if (UFDTArr[i].ptrfiletable != NULL)
        {
            if (strcmp((UFDTArr[i].ptrfiletable->ptrinode->FileName), name) == 0)
            {
                break;
            }
            i++;
        }
    }
    if (i == MAXINODE)
    {
        return -1;
    }
    else
    {
        return i;
    }
}

PINODE Get_Inode(char *name)
{
    PINODE temp = head;
    int i = 0;

    if (name == NULL)
    {
        return NULL;
    }
    while (temp != NULL)
    {
        if (strcmp(name, temp->FileName) == 0)
        {
            break;
        }
        temp = temp->next;
    }
    return temp;
}

void CreateDILB()
{
    int i = 1;
    PINODE newn = NULL;
    PINODE temp = head;

    while (i <= MAXINODE)
    {
        newn = (PINODE)malloc(sizeof(INODE));

        newn->LinkCount = 0;
        newn->ReferenceCount = 0;
        newn->FileType = 0;
        newn->FileSize = 0;
        newn->Buffer = NULL;
        newn->next = NULL;
        newn->InodeNumber = i;

        if (temp == NULL)
        {
            head = newn;
            temp = head;
        }
        else
        {
            temp->next = newn;
            temp = temp->next;
        }
        i++;
    }
    printf("DILB created successfully\n");
}

void IntialiseSuperBlock()
{
    int i=0;
    while(i<MAXINODE)
    {
        UFDTArr[i].ptrfiletable=NULL;
        i++;
    }
    SUPERBLOCKobj.TotalInodes=MAXINODE;
    SUPERBLOCKobj.FreeInode=MAXINODE;
}

int CreateFile(char *name,int permission)
{
    int i=0;
    PINODE temp = head;

    if((name==NULL)||(permission==0)||(permission>3))
    {
        return -1;
    }
    if(SUPERBLOCKobj.FreeInode==0)
    {
        return -2;
    }

    (SUPERBLOCKobj.FreeInode)--;

    if(Get_Inode(name)!=NULL) // To check file duplicate file is existring 
    {
        return -3;
    }

    while(temp!=NULL)
    {
        if(temp->FileType==0)
        {
            break;
        }
        temp=temp->next;
    }

    while(i<MAXINODE)
    {
        if(UFDTArr[i].ptrfiletable==NULL)
        {
            break;
        }
        i++;
    }

    UFDTArr[i].ptrfiletable=(PFILETABLE)malloc(sizeof(FILETABLE));

    UFDTArr[i].ptrfiletable->count=1;
    UFDTArr[i].ptrfiletable->mode=permission;
    UFDTArr[i].ptrfiletable->readoffset=0;
    UFDTArr[i].ptrfiletable->writeoffset=0;

    UFDTArr[i].ptrfiletable->ptrinode=temp;
    strcpy(UFDTArr[i].ptrfiletable->ptrinode->FileName,name);
    UFDTArr[i].ptrfiletable->ptrinode->FileType=REGULAR;
    UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount=1;
    UFDTArr[i].ptrfiletable->ptrinode->LinkCount=1;
    UFDTArr[i].ptrfiletable->ptrinode->FileSize=MAXFILESIZE;
    UFDTArr[i].ptrfiletable->ptrinode->permission=permission;
    UFDTArr[i].ptrfiletable->ptrinode->Buffer=(char*)malloc(MAXFILESIZE); //memory allocation for data

    return i;
}

int rm_File(char *name)
{
    int fd=0;
    fd=GetFDFromName(name);
    if(fd==-1)
    {
        return -1;
    }
    (UFDTArr[fd].ptrfiletable->ptrinode->LinkCount)--;

    if(UFDTArr[fd].ptrfiletable->ptrinode->LinkCount==0)
    {
        UFDTArr[fd].ptrfiletable->ptrinode->FileType=0;
        free(UFDTArr[fd].ptrfiletable->ptrinode->Buffer);
        free(UFDTArr[fd].ptrfiletable);
    }
    UFDTArr[fd].ptrfiletable=NULL;
    (SUPERBLOCKobj.FreeInode)++;

}

int ReadFile(int fd,char *arr,int isize)
{
    int read_size = 0;
    if(UFDTArr[fd].ptrfiletable==NULL)
    {
        return -1;
    }

    if(UFDTArr[fd].ptrfiletable->mode!= READ && UFDTArr[fd].ptrfiletable->mode != READ+WRITE)
    {
        return -2;
    }

    if(UFDTArr[fd].ptrfiletable->ptrinode->permission!= READ && UFDTArr[fd].ptrfiletable->ptrinode->permission != READ+WRITE)
    {
        return -2;
    }

    if(UFDTArr[fd].ptrfiletable->readoffset==UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)
    {
        return -3;
    }

    if(UFDTArr[fd].ptrfiletable->ptrinode->FileType != REGULAR)
    {
        return -4;
    }

    read_size=(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)-(UFDTArr[fd].ptrfiletable->readoffset);

    if(read_size<isize)
    {
        strncpy(arr,(UFDTArr[fd].ptrfiletable->ptrinode->Buffer)+(UFDTArr[fd].ptrfiletable->readoffset),read_size);

        UFDTArr[fd].ptrfiletable->readoffset=UFDTArr[fd].ptrfiletable->readoffset+read_size;
    }
    else
    {
        strncpy(arr,(UFDTArr[fd].ptrfiletable->ptrinode->Buffer)+(UFDTArr[fd].ptrfiletable->readoffset),isize);

        (UFDTArr[fd].ptrfiletable->readoffset)=(UFDTArr[fd].ptrfiletable->readoffset)+isize;
    }

    return isize;  

}

int WriteFile(int fd,char *arr,int isize)
{
    if(((UFDTArr[fd].ptrfiletable->mode)!=WRITE)&&((UFDTArr[fd].ptrfiletable->mode)!=READ+WRITE))
    {
        return -1;
    }

    if(((UFDTArr[fd].ptrfiletable->ptrinode->permission)!=WRITE)&&((UFDTArr[fd].ptrfiletable->ptrinode->permission)!=READ+WRITE))
    {
        return -1;
    }

    if((UFDTArr[fd].ptrfiletable->writeoffset)==MAXFILESIZE)
    {
        return -2;
    }

    if((UFDTArr[fd].ptrfiletable->ptrinode->FileType)!=REGULAR)
    {
        return -3;
    }

    strncpy((UFDTArr[fd].ptrfiletable->ptrinode->Buffer)+(UFDTArr[fd].ptrfiletable->writeoffset),arr,isize);
    
    (UFDTArr[fd].ptrfiletable->writeoffset)=(UFDTArr[fd].ptrfiletable->writeoffset)+isize;

    (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)=(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)+isize;

    return isize;
}

int OpenFile(char *name,int mode)
{
    int i=0;
    PINODE temp =NULL;

    if(name==NULL||mode<=0)
    {
        return -1;
    }

    temp=Get_Inode(name);

    if(temp==NULL)
    {
        return -2;
    }

    if(temp->permission<mode)
    {
        return -3;
    }

    while (i<MAXINODE)
    {
        if(UFDTArr[i].ptrfiletable==NULL)
        break;
        i++;
    }

    UFDTArr[i].ptrfiletable=(PFILETABLE)malloc(sizeof(FILETABLE));
    if(UFDTArr[i].ptrfiletable==NULL)
    {
        return -1;
    }
    UFDTArr[i].ptrfiletable->count=1;
    UFDTArr[i].ptrfiletable->mode=mode;

    if(mode==READ+WRITE)
    {
        UFDTArr[i].ptrfiletable->readoffset=0;
        UFDTArr[i].ptrfiletable->writeoffset=0;
    }
    else if(mode==READ)
    {
        UFDTArr[i].ptrfiletable->writeoffset=0;
    }
    else if(mode == WRITE)
    {
        UFDTArr[i].ptrfiletable->writeoffset=0;
    }
    
    UFDTArr[i].ptrfiletable->ptrinode=temp;
    (UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)++;

    return i;
    
}

void CloseFileByName(int fd)
{
    UFDTArr[fd].ptrfiletable->readoffset=0;
    UFDTArr[fd].ptrfiletable->writeoffset=0;
    (UFDTArr[fd].ptrfiletable->ptrinode->ReferenceCount)--;
}

int CloaseFileByName(char *name)
{
    int i=0;
    i=GetFDFromName(name);

    if(i==-1)
    {
        return -1;
    }

    UFDTArr[i].ptrfiletable->readoffset=0;
    UFDTArr[i].ptrfiletable->writeoffset=0;
    (UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)--;

    return 0;
}

void CloseAllFile()
{
    int i=0;
    while(i<MAXINODE)
    {
        if(UFDTArr[i].ptrfiletable!=NULL)
        {
            UFDTArr[i].ptrfiletable->readoffset=0;
            UFDTArr[i].ptrfiletable->writeoffset=0;
            (UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)--;
            break;
        }
        i++;
    }
}

int LseekFile(int fd,int size,int from)
{
    if((fd<0)||(from>2))
    {
        return -1;
    }

    if(UFDTArr[fd].ptrfiletable==NULL)
    {
        return -1;
    }

    if((UFDTArr[fd].ptrfiletable->mode==READ)||(UFDTArr[fd].ptrfiletable->mode==READ+WRITE))
    {
        if(from==CURRENT)
        {
            if(((UFDTArr[fd].ptrfiletable->readoffset)+size)>UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)
            {
                return -1;
            }
            if(((UFDTArr[fd].ptrfiletable->readoffset)+size)<0)
            {
                return -1;
            }
            (UFDTArr[fd].ptrfiletable->readoffset)=(UFDTArr[fd].ptrfiletable->readoffset)+size;
        }
        else if(from==START)
        {
            if(size>(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))
            {
                return -1;
            }
            if(size<0)
            {
                return -1;
            }
            (UFDTArr[fd].ptrfiletable->readoffset)=size;
        }
        else if(from == END)
        {
            if((UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)+size>MAXFILESIZE)
            {
                return -1;
            }
            if(((UFDTArr[fd].ptrfiletable->readoffset)+size)<0)
            {
                return -1;
            }
            (UFDTArr[fd].ptrfiletable->readoffset)=(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)+size;
        }
    }
    // Continue from line 458 in pdf
}

int main()
{
    char Arr[50];
    // while(1){
    //     strcpy(Arr,"");
    // printf("Enter Comand: ");
    // gets(Arr);
    // man(Arr);
    // }
    // DisplayHelp();
    IntialiseSuperBlock();
    CreateDILB();
    
}