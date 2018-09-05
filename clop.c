#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#ifdef __linux__
	#include <sys/utsname.h>
#endif

#define VERSION "1.03"

#ifdef _WIN64
    const char* OS = "Windows 64-bit";
#elif _WIN32
    const char* OS = "Windows 32-bit";
#elif __APPLE__ || __MACH__
    const char* OS = "Mac OS X";
#elif __FreeBSD__
    const char* OS = "FreeBSD";
#else
    const char* OS = "Other";
#endif

#define max(a,b) ((a) > (b) ? a : b)
#define min(a,b) ((a) < (b) ? a : b)

// progress - режим із відображенням прогресу
int progress = 0;
// output - відправити stdout у файл out_%
int output = 0;

const int Step = 2;
const uint64_t minPrime = 3;
const uint64_t maxPrime = UINT64_MAX;

struct timeb starttime, endtime;
FILE * fout, * fchk;

// 8GB обмеження по пам'яті
uint32_t maxBlockSize = 1024 * 1024 * 1024, blockSize = 100000, bSize = 0;
uint64_t * Block = NULL;
uint64_t toCnt = 0;

uint64_t Ini, Fin, Cur, taskIni, taskFin;
char outfname[256] = "out", chkfname[256] = "chk";

static __inline__ uint64_t string_to_u64(const char * s) {
  uint64_t i;
  char c ;
  int scanned = sscanf(s, "%" SCNu64 "%c", &i, &c);
  if (scanned == 1) return i;
  if (scanned > 1) {
    // TBD about extra data found
    return i;
    }
  // TBD failed to scan;
  return 0;
}

void save_checkpoint(uint64_t pos)
{
    fchk = fopen(chkfname, "w");
    if(fchk == NULL) {
        exit(EXIT_FAILURE);
    }

    struct timeb curtime;
    ftime(&curtime);
    uint64_t dif = (curtime.time - starttime.time) * 1000 + (curtime.millitm - starttime.millitm);
    fprintf(fchk,  "%" PRIu64
                  ",%" PRIu64
                  ",%" PRIu64
                  ",%" PRIu64
                  ",%" PRIu64
                ,Ini
                ,Fin
                ,pos
                ,dif
                ,toCnt
           );
    fflush(fchk);
    fclose(fchk);
}

int read_checkpoint(void)
{
    fchk = fopen(chkfname, "r");
    if(fchk == NULL)
        return (EXIT_FAILURE);
    char c;
    uint64_t dif;
    int scanned = fscanf(fchk, "%" SCNu64
                              ",%" SCNu64
                              ",%" SCNu64
                              ",%" SCNu64
                              ",%" SCNu64
                              ",%c"
                              , &Ini
                              , &Fin
                              , &Cur
                              , &dif
                              , &toCnt
                              , &c);
    fclose(fchk);
    if (scanned != 5) {
        exit(EXIT_FAILURE);
    }
    if (!Cur) return 1;
        else Cur = ((Cur / Step) + 1) * Step + 1;
    starttime.time -= dif / 1000;
    long int millisec = (dif % 1000);
    if (starttime.millitm < millisec) {
        starttime.millitm += 1000 - millisec;
        starttime.time--;
    } else starttime.millitm -= millisec;
    return 0;
}

void free_block(void)
{
    free(Block);
}

void sieve_block(uint32_t size, uint64_t last)
{
    uint64_t i, maxFactor = rintl(sqrtl(last));
    if (bSize - size) {
        free_block();
        Block = (uint64_t *) calloc(size, sizeof(uint64_t));
        bSize = size;
    }
    else {
        for (i = 0; i < size; i++) Block[i] = 0;
    }
    for (i = 3; i <= maxFactor; i += 2) {
        uint64_t j, k = Cur / i;
        k = i * max(3, ((k && 1) && !(Cur % i) ? k : ((((k + 1) >> 1) << 1) + 1)));
        for (j = k; (j <= last) && (Cur <= j); j += 2*(uint64_t)i) {
            Block[(j - Cur) >> 7] |= ((uint64_t)1 << (((j - Cur) >> 1) & 63));
        }
    }
}

int init_task(void)
{
    if (Ini > Fin) return 1;
    if (Ini < minPrime) Ini = minPrime;
    if (Fin > maxPrime) Fin = maxPrime;
    Ini = (uint64_t)(Ini / Step) * Step + 1;
    Fin = (uint64_t)((Fin - 1) / Step) * Step + 1;
    Cur = Ini;
    return 0;
}

#define PBSTR "========================================================================"
#define PBWIDTH 55
#define SCRWIDTH 80
void do_progress( double percentage )
{
    int val = (int) (percentage);
    int lpad = (int) (percentage  * (val==100?SCRWIDTH:PBWIDTH) / 100);
    int rpad = (val==100?SCRWIDTH:PBWIDTH) - lpad;
    //fill progress bar with spaces
    fprintf(stderr, "\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    if (val!=100)
        fprintf(stderr, " %" PRIu64, toCnt);
    //fflush(stderr);
}

void print_usage(void)
{
#ifdef _WIN32
	char pref[3] = "";
#elif __linux__ || unix || __unix__ || __unix
	char pref[3] = "./";
#endif // __linux__
    fprintf(stderr, "Usage: %sclop <low> <high> [switches]\n", pref);
    fprintf(stderr, "\t<low>\t\tlower border\n");
    fprintf(stderr, "\t<high>\t\thigher border\n");
    fprintf(stderr, "The following switches are accepted:\n");
    fprintf(stderr, "\t-p\t\tdisplay progress bar\n");
    fprintf(stderr, "\t-o [fn]\t\twrite results to [fn] file\n");
    fprintf(stderr, "\t-f [s]\t\tfactoring block size (default value: %" PRIu32 ")\n", blockSize);
}

int main(int argc, char** argv)
{
#ifdef _WIN32
#elif __linux__ || unix || __unix__ || __unix
	char OS[256];
	struct utsname name;
	if(uname(&name)) exit(EXIT_FAILURE);
	sprintf(OS, "%s %s", name.sysname, name.release);
#endif // __linux__
    fprintf(stderr, "CLOP (Consecutive List Of Primes) %s (%s)\nCopyright 2018, Alexander Belogourov aka x3mEn\n\n", VERSION, OS);
    if (argc < 3) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    taskIni = Ini = string_to_u64(argv[1]);
    taskFin = Fin = string_to_u64(argv[2]);
    if (!Ini || !Fin) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    sprintf(outfname, "out_%019" PRIu64 "_%019" PRIu64, taskIni, taskFin);
    sprintf(chkfname, "chk_%019" PRIu64 "_%019" PRIu64, taskIni, taskFin);

    for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[i],"-p")) {progress = 1; continue;}
        if (!strcmp(argv[i],"-o")) {output = 1; continue;}
        if (argv[i][0] != '-' && !strcmp(argv[i-1],"-o")) {sprintf(outfname, argv[i]); continue;}
        if (!strcmp(argv[i],"-f")) {continue;}
        if (string_to_u64(argv[i]) && !strcmp(argv[i-1],"-f")) {blockSize = min(maxBlockSize, string_to_u64(argv[i])); continue;}
        print_usage();
        exit(EXIT_FAILURE);
    }

    ftime(&starttime);

    time_t timer;
    char curdatetime[26];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(curdatetime, 26, "%d.%m.%Y %H:%M:%S", tm_info);

    int ErrorCode, CheckPointCode;
    ErrorCode = CheckPointCode = read_checkpoint();
    if (ErrorCode) ErrorCode = init_task();
    if (ErrorCode) return ErrorCode;

    uint64_t total = Fin >= Ini ? (uint64_t)((Fin - Ini) / Step) + 1 : 0;
    uint64_t bStep = 0, blockWidth = (blockSize - 1) * Step;

    fout = fopen(outfname, "r");
    if (fout != NULL && CheckPointCode) {
        fclose(fout);
        exit (EXIT_SUCCESS);
    }
    if (output) {
        if (!CheckPointCode && fout == NULL) {
            exit(EXIT_FAILURE);
        }
        if (CheckPointCode) {
            fout = fopen(outfname, "w");
        } else {
            fout = fopen(outfname, "a");
        }
        if (fout == NULL) {
            exit(EXIT_FAILURE);
        }
    }

    fprintf(stderr, "Command line parameters :");
    for (int i = 1; i < argc; i++)
        fprintf(stderr, " %s", argv[i]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Number range borders    : from %" PRIu64 " to %" PRIu64 " step %d\n", Ini, Fin, Step);
    fprintf(stderr, "Total amount of Numbers : %" PRIu64 "\n", total);
    fprintf(stderr, "Factoring Block Size    : %" PRIu32 "\n", blockSize);
    fprintf(stderr, "Starting date timestamp : %s\n", curdatetime);

    int cpcnt, ctpcnt = 0;
    float cstep = 1000;

    if (progress)
        do_progress(ctpcnt);

    if (taskIni < minPrime) {
        toCnt++;
        if (output) fprintf(fout, "%d\n", 2);
    }

    while (Ini <= Cur && Cur <= Fin) {
        uint32_t bs = (Fin - Cur < blockWidth ? Fin - Cur : blockWidth) / Step + 1;
        uint64_t Last = Cur + (bs - 1) * Step;
        sieve_block(bs, Last);
        for (uint64_t i = 0; i < bSize * Step; i += 2) {
            if (!(Block[i >> 7]&((uint64_t)1 << ((i >> 1) & 63)))) {
                toCnt++;
                if (output) fprintf(fout, "%" PRIu64 "\n", Cur + i);
            }
        }
		bStep = (Last - Ini) / Step + 1;
		cpcnt = (int)((double)bStep / total * cstep);
		if (ctpcnt != cpcnt) {
            ctpcnt = cpcnt;
			if (progress)
				do_progress((double)bStep / total * 100);
			save_checkpoint(Last + 1);
			if (output) fflush(fout);
			fflush(stdout);
		}
        Cur += bSize * Step;
    };

    if (output) fclose(fout);
    remove(chkfname);

    ftime(&endtime);
    uint64_t dif = (endtime.time - starttime.time) * 1000 + (endtime.millitm - starttime.millitm);

    fprintf(stderr, "\n");
    fprintf(stderr, "Elapsed time            : %02d:%02d:%02d.%03d\n", (unsigned char)(dif/60/60/1000), (unsigned char)((dif/60/1000)%60), (unsigned char)((dif/1000)%60), (unsigned char)(dif%1000));
    fprintf(stderr, "Total amount of Primes  : %" PRIu64 "\n", toCnt);
    free_block();
    return (EXIT_SUCCESS);
}
