#include <stdio.h>
#include <unistd.h>
int main(){
    int a[6];
    for(int i=0;i<6;i++){
        scanf("%d",&a[i]);
    }
    pid_t pid=fork();
    if(pid<0){
        printf("process not created\n");
    }
    else if(pid>0){
        int sum=0;
        for(int i=0;i<6;i++){
            sum=sum+a[i];
        }
        printf("sum of array is %d",sum);
    }
    else{
        int pro=0;
        for(int i=0;i<6;i++){
            pro=pro*a[i];
        }
        printf("product of array is %d",pro);
    }
}