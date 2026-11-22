/* vulp.c
 *
 * Deliberately vulnerable Set-UID-root program used in the
 * SEED Labs "Race Condition Vulnerability Lab" (Wenliang Du).
 * Distributed here as part of a CyberRangeCZ Platform training
 * scenario, for non-commercial educational use only.
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main()
{
    char * fn = "/tmp/XYZ";
    char buffer[60];
    FILE *fp;

    /* get user input */
    scanf("%50s", buffer);

    if (!access(fn, W_OK)) {               /* (1) check */
        fp = fopen(fn, "a+");               /* (2) use   */
        fwrite("\n", sizeof(char), 1, fp);
        fwrite(buffer, sizeof(char), strlen(buffer), fp);
        fclose(fp);
    } else {
        printf("No permission \n");
    }

    return 0;
}
