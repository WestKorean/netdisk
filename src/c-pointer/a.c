#include <stdio.h>
#include <stdlib.h>


int main()
{
    int *p1;
    int *p2;
    int a[] = {1,2,3,4,5};
 //   p1 = (int *) (&a + 1);
    
   // int b = *(a + 1);
   // int c = *(p1 - 1);

    printf("a: 0x%x  &a: 0x%x \n",&a  ,&a + 1);
    

    exit(0);

}
