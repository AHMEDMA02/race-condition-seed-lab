/* vulp_seteuid.c -- skeleton for Level 5 (Principle of Least Privilege)
 *
 * Starting point for applying the Principle of Least Privilege to vulp.c.
 *
 * Think carefully about WHERE root privilege is actually needed here.
 * vulp only ever appends to a file that the real (unprivileged) user is
 * already allowed to write (that is the whole point of the access()
 * check). So does this program need root privilege at all, for the
 * access()/fopen()/fwrite()/fclose() sequence below? If it does not,
 * the safest fix is to drop the effective privilege once, before any
 * of that sequence runs, and simply never raise it again for this
 * operation -- rather than dropping and re-raising it, which would
 * reopen the same race-condition window you exploited in Level 4.
 *
 * TODO (this is your task, not a finished exercise):
 *   1. Decide where effective privilege should be dropped with
 *      seteuid(getuid()), and justify your placement in the lab
 *      report / solution notes.
 *   2. Recompile as vulp_fixed, re-apply the Set-UID bit with the
 *      whitelisted sudo commands, and repeat your Level 4 attack
 *      against this new binary.
 *   3. Record exactly what happens and why.
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main()
{
    char * fn = "/tmp/XYZ";
    char buffer[60];
    FILE *fp;

    scanf("%50s", buffer);

    /* TODO: drop effective privilege here, with seteuid(getuid()) */

    if (!access(fn, W_OK)) {
        fp = fopen(fn, "a+");
        fwrite("\n", sizeof(char), 1, fp);
        fwrite(buffer, sizeof(char), strlen(buffer), fp);
        fclose(fp);
    } else {
        printf("No permission \n");
    }

    return 0;
}
