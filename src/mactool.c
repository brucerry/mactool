#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <mtd/mtd-abi.h>
#include <linux/version.h>
#include <dirent.h>

typedef enum
{
    OP_NONE = -1,
    OP_GET,
    OP_SET,
} OPERATION_CODE;

typedef enum
{
    MAC_FOR_NONE = -1,
    MAC_FOR_WIFI0,
    MAC_FOR_WIFI1,
    MAC_FOR_WIFI2,
    MAC_FOR_WIFI3,
    MAC_FOR_ETH0,
    MAC_FOR_ETH1,
    MAC_FOR_ETH2,
    MAC_FOR_ETH3,
    MAC_FOR_ETH4,
    MAC_FOR_ETH5,
} MAC_FOR;

#define DEBUG                   0

#define MTD_PATH                "/proc/mtd"
#define MMC_PATH                "/sys/block/mmcblk0"

/*
 *  LAN interface 
 */
#define ETH0_MAC_OFFSET         0x0
#define ETH1_MAC_OFFSET         0x6
#define ETH2_MAC_OFFSET         0xc
#define ETH3_MAC_OFFSET         0x12
#define ETH4_MAC_OFFSET         0x18
#define ETH5_MAC_OFFSET         0x1e

/*
 *  WLAN interface 
 */
#define WLAN0_CHK_CALC_SIZE     184320
#define WLAN1_CHK_CALC_SIZE     184320
#define WLAN2_CHK_CALC_SIZE     184320
#define WLAN3_CHK_CALC_SIZE     184320
#define WLAN0_CHK_CALC          0x26800
#define WLAN1_CHK_CALC          0x58800
#define WLAN2_CHK_CALC          0x8A800
#define WLAN3_CHK_CALC          0xBC800

#define WLAN0_CHK_OFFSET        (WLAN0_CHK_CALC+12)
#define WLAN1_CHK_OFFSET        (WLAN1_CHK_CALC+12)
#define WLAN2_CHK_OFFSET        (WLAN2_CHK_CALC+12)
#define WLAN3_CHK_OFFSET        (WLAN3_CHK_CALC+12)
#define WLAN0_MAC_OFFSET        (WLAN0_CHK_CALC+16)
#define WLAN1_MAC_OFFSET        (WLAN1_CHK_CALC+16)
#define WLAN2_MAC_OFFSET        (WLAN2_CHK_CALC+16)
#define WLAN3_MAC_OFFSET        (WLAN3_CHK_CALC+16)
#define WLAN0_NVMACFLAG_OFFSET  (WLAN0_CHK_CALC+63)
#define WLAN1_NVMACFLAG_OFFSET  (WLAN1_CHK_CALC+63)
#define WLAN2_NVMACFLAG_OFFSET  (WLAN2_CHK_CALC+63)
#define WLAN3_NVMACFLAG_OFFSET  (WLAN3_CHK_CALC+63)
#define WLAN0_NUMMACADDR_OFFSET (WLAN0_CHK_CALC+537)
#define WLAN1_NUMMACADDR_OFFSET (WLAN1_CHK_CALC+537)
#define WLAN2_NUMMACADDR_OFFSET (WLAN2_CHK_CALC+537)
#define WLAN3_NUMMACADDR_OFFSET (WLAN3_CHK_CALC+537)




#define max(a,b)                ({ __typeof__ (a) _a = (a);__typeof__ (b) _b = (b);  _a > _b ? _a : _b; })


static int is_emmc_only = 0;


/*
 * MEMGETINFO
 */
static int getmeminfo(int fd, struct mtd_info_user *mtd)
{
    return (ioctl(fd, MEMGETINFO, mtd));
}

/*
 * MEMERASE
 */
static int memerase(int fd, struct erase_info_user *erase)
{
    return (ioctl(fd, MEMERASE, erase));
}

static int erase_flash(int fd, uint32_t offset, uint32_t bytes)
{
    int err;
    struct erase_info_user erase;
    erase.start = offset;
    erase.length = bytes;
    err = memerase (fd,&erase);
    if (err < 0)
    {
        perror ("MEMERASE");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


static void cal_checksum(uint8_t* buf, int calcAddr, int mac_for) {
    uint16_t *calcbuf = (uint16_t *)&buf[calcAddr];
    uint16_t checksum = 0;

    int calbuf_size = 0;

#ifdef WLAN0_CHK_CALC_SIZE
    if (mac_for == MAC_FOR_WIFI0)
        calbuf_size = WLAN0_CHK_CALC_SIZE;
#endif

#ifdef WLAN1_CHK_CALC_SIZE
    if (mac_for == MAC_FOR_WIFI1)
        calbuf_size = WLAN1_CHK_CALC_SIZE;
#endif

#ifdef WLAN2_CHK_CALC_SIZE
    if (mac_for == MAC_FOR_WIFI2)
        calbuf_size = WLAN2_CHK_CALC_SIZE;
#endif

#ifdef WLAN3_CHK_CALC_SIZE
    if (mac_for == MAC_FOR_WIFI3)
        calbuf_size = WLAN3_CHK_CALC_SIZE;
#endif

#ifdef WLAN0_MAC_OFFSET
    int wlan0ChkAddr = WLAN0_CHK_OFFSET;
#endif
#ifdef WLAN1_MAC_OFFSET
    int wlan1ChkAddr = WLAN1_CHK_OFFSET;
#endif
#ifdef WLAN2_MAC_OFFSET
    int wlan2ChkAddr = WLAN2_CHK_OFFSET;
#endif
#ifdef WLAN3_MAC_OFFSET
    int wlan3ChkAddr = WLAN3_CHK_OFFSET;
#endif

    for (int i = 0; i < (calbuf_size/2); i++)
        checksum ^= calcbuf[i];
    
    uint8_t checksum_H = ((checksum >> 8) & 0xff);
    uint8_t checksum_L = checksum & 0xff;
#if DEBUG
    printf("checksum_L = 0x%02X\n", checksum_L);
    printf("checksum_H = 0x%02X\n", checksum_H);
#endif
    
#ifdef WLAN0_MAC_OFFSET    
    if (mac_for == MAC_FOR_WIFI0) {
        buf[wlan0ChkAddr] = checksum_L;
        buf[wlan0ChkAddr+1] = checksum_H;
    }
#endif
#ifdef WLAN1_MAC_OFFSET    
    if (mac_for == MAC_FOR_WIFI1) {
        buf[wlan1ChkAddr] = checksum_L;
        buf[wlan1ChkAddr+1] = checksum_H;
    }
#endif
#ifdef WLAN2_MAC_OFFSET
    if (mac_for == MAC_FOR_WIFI2) {
        buf[wlan2ChkAddr] = checksum_L;
        buf[wlan2ChkAddr+1] = checksum_H;
    }
#endif
#ifdef WLAN3_MAC_OFFSET
    if (mac_for == MAC_FOR_WIFI3) {
        buf[wlan3ChkAddr] = checksum_L;
        buf[wlan3ChkAddr+1] = checksum_H;
    }
#endif
}

static int verify_interface(char* interface)
{
    if (!interface)
    {
        printf("  ## %s() FAIL: Interface is NULL\n", __FUNCTION__);
        return EXIT_FAILURE;
    }

    int matched = 0;
    /*
    *  LAN Interface
    */
#ifdef ETH0_MAC_OFFSET
    matched |= strcasecmp(interface, "eth0") == 0;
#endif
#ifdef ETH1_MAC_OFFSET
    matched |= strcasecmp(interface, "eth1") == 0;
#endif
#ifdef ETH2_MAC_OFFSET
    matched |= strcasecmp(interface, "eth2") == 0;
#endif
#ifdef ETH3_MAC_OFFSET
    matched |= strcasecmp(interface, "eth3") == 0;
#endif
#ifdef ETH4_MAC_OFFSET
    matched |= strcasecmp(interface, "eth4") == 0;
#endif
#ifdef ETH5_MAC_OFFSET
    matched |= strcasecmp(interface, "eth5") == 0;
#endif

    /*
    *  WLAN Interface
    */
#ifdef WLAN0_MAC_OFFSET
    matched |= strcasecmp(interface, "wifi0") == 0;
#endif
#ifdef WLAN1_MAC_OFFSET
    matched |= strcasecmp(interface, "wifi1") == 0;
#endif
#ifdef WLAN2_MAC_OFFSET
    matched |= strcasecmp(interface, "wifi2") == 0;
#endif
#ifdef WLAN3_MAC_OFFSET
    matched |= strcasecmp(interface, "wifi3") == 0;
#endif

    if (!matched)
    {
        printf("  ## %s() FAIL: Do not support interface \"%s\"\n", __FUNCTION__, interface);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int verify_mac_address(char* mac_address)
{
    if (!mac_address)
    {
        printf("  ## %s() FAIL: MAC address is NULL\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    int len = strlen(mac_address);
    if (len != 12)
    {
        printf("  ## %s() FAIL: MAC address length = %d. Required 12\n", __FUNCTION__, len);
        return EXIT_FAILURE;
    }
    int char_err = 0;
    for (int i = 0; i < len; ++i)
    {
        if (!isalnum(mac_address[i]))
        {
            printf("  ## %s() FAIL: MAC address index %d = '%c' is an invalid character\n", __FUNCTION__, i, mac_address[i]);
            char_err = 1;
        }
        if (isalpha(mac_address[i]))
        {
            if (isupper(mac_address[i]) && mac_address[i] > 'F')
            {
                printf("  ## %s() FAIL: MAC address index %d = '%c' is not in HEX range ['A', 'F']\n", __FUNCTION__, i, mac_address[i]);
                char_err = 1;
            }
            if (islower(mac_address[i]) && mac_address[i] > 'f')
            {
                printf("  ## %s() FAIL: MAC address index %d = '%c' is not in HEX range ['a', 'f']\n", __FUNCTION__, i, mac_address[i]);
                char_err = 1;
            }
        }
    }
    return char_err ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int verify_option(int is_get, int is_set, char* interface, char* mac_address)
{
    if (is_get && is_set)
    {
        printf("  ## %s() FAIL: Do not support Get & Set in a row\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else if (is_get)
    {
        if (verify_interface(interface))
        {
            printf("  ## %s() FAIL\n", __FUNCTION__);
            return EXIT_FAILURE;
        }
    }
    else if (is_set)
    {
        if (verify_interface(interface))
        {
            printf("  ## %s() FAIL\n", __FUNCTION__);
            return EXIT_FAILURE;
        }
        if (verify_mac_address(mac_address))
        {
            printf("  ## %s() FAIL\n", __FUNCTION__);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static void print_usage(char* program_name)
{
    printf("Usage:\n");
    printf("\n");
    printf("%s [-e] [-i wifi#] [-g] [-s ############] [-h]\n", program_name);
    printf("\n");
    printf("-e: A eMMC-only device. Declare that ART partition is stored in eMMC\n");
    printf("-i: Declare the LAN/WLAN interface\n");
    printf("-g: Get the MAC address\n");
    printf("-s: Set the MAC address\n");
    printf("-h: Print usage\n");
    printf("\n");
    printf("Example:\n");
    printf("%s -i eth0 -g                # Get the eth0 MAC address\n", program_name);
    printf("%s -i wifi0 -g               # Get the wifi0 MAC address\n", program_name);
    printf("%s -i eth0 -s 4c130465000a   # Set the eth0 MAC address as 4c:13:04:65:00:0a\n", program_name);
    printf("%s -i wifi0 -s 4c130465000b  # Set the wifi0 MAC address as 4c:13:04:65:00:0b\n", program_name);
    printf("%s -i eth0 -g -e             # Get the eth0 MAC address from eMMC ART partition\n", program_name);
    printf("%s -i wifi0 -g -e            # Get the wifi0 MAC address from eMMC ART partition\n", program_name);
}

/**
 * Find MTD ART partition
 * Return a file descriptor
 */
static int find_mtd_art()
{
    FILE* art_file = NULL;
    char buf[256] = "";
    char artmtd[256] = "/dev/";
    char *tok = "";
    int fd = -1;
    char* path = MTD_PATH;

    if ((art_file = fopen(path, "r")) <= 0)
    {
        printf("  ## %s() FAIL: fopen() -> %s error\n", __FUNCTION__, path);
        return fd;
    }

    printf("  ## %s() SUCCESS: fopen() -> %s\n", __FUNCTION__, path);

    while (fgets(buf, sizeof(buf), art_file) != NULL )
    {
        if (strstr(buf, "ART") || strstr(buf, "art"))
        {
            tok = strtok(buf, ":");
            strcat(artmtd, tok);
            break;
        }
    }
    fclose(art_file);

    if ((fd = open(artmtd, O_SYNC | O_RDWR)) < 0)
    {
        printf("  ## %s() FAIL: fopen() -> %s error\n", __FUNCTION__, artmtd);
        return fd;
    }

    printf("  ## %s() SUCCESS: fopen() -> %s\n", __FUNCTION__, artmtd);

    return fd;
}

/**
 * Lists all files and sub-directories recursively 
 * considering path as base path.
 */
static void found_art_mmc_recursion(char* artmmc, char *basePath, char* dirName)
{
    struct dirent *dp;
    DIR *dir = opendir(basePath);

    // Unable to open directory stream
    if (!dir)
        return;

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            if (dp->d_type == DT_DIR)
            {
                char path[1024];
                strcpy(path, basePath);
                strcat(path, "/");
                strcat(path, dp->d_name);
                // printf("Searching next path %s\n", path);
                found_art_mmc_recursion(artmmc, path, dp->d_name);
            }
            else
            {
                int found = 0;
                char full_path[1024];
                strcpy(full_path, basePath);
                strcat(full_path, "/");
                strcat(full_path, dp->d_name);
                FILE* file = fopen(full_path, "r");
                // printf("  Reading file %s\n", full_path);
                char buf[256];
                if (file)
                {
                    while (fgets(buf, sizeof(buf), file))
                    {
                        if(strstr(buf, "0:ART") || strstr(buf, "0:art"))
                        {
                            strcat(artmmc, dirName);
                            found = 1;
                            printf("  ## Found MMC ART partition at %s\n", full_path);
                            break;
                        }
                    }
                    fclose(file);
                }
                if (found)
                    break;
            }
        }
    }

    closedir(dir);
}

/**
 * Find MMC ART partition
 * Return a file descriptor
 */
static int find_mmc_art()
{
    char artmmc[256] = "/dev/";
    int fd = -1;

    found_art_mmc_recursion(artmmc, MMC_PATH, NULL);

    if ((fd = open(artmmc, O_SYNC | O_RDWR)) < 0)
    {
        printf("  ## %s() FAIL: fopen() -> %s error\n", __FUNCTION__, artmmc);
        return fd;
    }

    printf("  ## %s() SUCCESS: fopen() -> %s\n", __FUNCTION__, artmmc);

    return fd;
}

void mac_convert(uint8_t* buf, char* mac, int mac_for)
{

/*
 *  LAN interface 
 */
#ifdef ETH0_MAC_OFFSET
    int eth0MacAddr = ETH0_MAC_OFFSET;
#endif
#ifdef ETH1_MAC_OFFSET
    int eth1MacAddr = ETH1_MAC_OFFSET;
#endif
#ifdef ETH2_MAC_OFFSET
    int eth2MacAddr = ETH2_MAC_OFFSET;
#endif
#ifdef ETH3_MAC_OFFSET
    int eth3MacAddr = ETH3_MAC_OFFSET;
#endif
#ifdef ETH4_MAC_OFFSET
    int eth4MacAddr = ETH4_MAC_OFFSET;
#endif
#ifdef ETH5_MAC_OFFSET
    int eth5MacAddr = ETH5_MAC_OFFSET;
#endif

/*
 *  WLAN interface 
 */
#ifdef WLAN0_MAC_OFFSET
    int wlan0ChkAddr = WLAN0_CHK_OFFSET;
    int wlan0MacAddr = WLAN0_MAC_OFFSET;
#endif
#ifdef WLAN1_MAC_OFFSET
    int wlan1ChkAddr = WLAN1_CHK_OFFSET;
    int wlan1MacAddr = WLAN1_MAC_OFFSET;
#endif
#ifdef WLAN2_MAC_OFFSET
    int wlan2ChkAddr = WLAN2_CHK_OFFSET;
    int wlan2MacAddr = WLAN2_MAC_OFFSET;
#endif
#ifdef WLAN3_MAC_OFFSET
    int wlan3ChkAddr = WLAN3_CHK_OFFSET;
    int wlan3MacAddr = WLAN3_MAC_OFFSET;
#endif

    for (int i = 0; i < 12; ++i)
    {
        mac[i] = isdigit(mac[i]) ? mac[i] - '0' : mac[i];
        mac[i] = isalpha(mac[i]) ? (islower(mac[i]) ? mac[i] - 'a' + 10 : mac[i] - 'A' + 10) : mac[i];
        
        if (i & 1)
        {
/*
 *  LAN interface 
 */
#ifdef ETH0_MAC_OFFSET            
            if (mac_for == MAC_FOR_ETH0)
                buf[i/2+eth0MacAddr]=(mac[i-1]<<4 | mac[i]);
#endif            
#ifdef ETH1_MAC_OFFSET            
            if (mac_for == MAC_FOR_ETH1)
                buf[i/2+eth1MacAddr]=(mac[i-1]<<4 | mac[i]);
#endif            
#ifdef ETH2_MAC_OFFSET            
            if (mac_for == MAC_FOR_ETH2)
                buf[i/2+eth2MacAddr]=(mac[i-1]<<4 | mac[i]);
#endif            
#ifdef ETH3_MAC_OFFSET            
            if (mac_for == MAC_FOR_ETH3)
                buf[i/2+eth3MacAddr]=(mac[i-1]<<4 | mac[i]);
#endif            
#ifdef ETH4_MAC_OFFSET            
            if (mac_for == MAC_FOR_ETH4)
                buf[i/2+eth4MacAddr]=(mac[i-1]<<4 | mac[i]);
#endif            
#ifdef ETH5_MAC_OFFSET            
            if (mac_for == MAC_FOR_ETH5)
                buf[i/2+eth5MacAddr]=(mac[i-1]<<4 | mac[i]);
#endif

/*
 *  WLAN interface 
 */
#ifdef WLAN0_MAC_OFFSET
            if (mac_for == MAC_FOR_WIFI0) {
                buf[i/2+wlan0MacAddr] = mac[i-1]<<4 | mac[i];
                buf[wlan0ChkAddr] = 0xff;
                buf[wlan0ChkAddr+1] = 0xff;
            }
#endif
#ifdef WLAN0_MAC_OFFSET
            if (mac_for == MAC_FOR_WIFI1) {
                buf[i/2+wlan1MacAddr] = mac[i-1]<<4 | mac[i];
                buf[wlan1ChkAddr] = 0xff;
                buf[wlan1ChkAddr+1] = 0xff;
            }
#endif
#ifdef WLAN2_MAC_OFFSET
            if (mac_for == MAC_FOR_WIFI2) {
                buf[i/2+wlan2MacAddr] = mac[i-1]<<4 | mac[i];
                buf[wlan2ChkAddr] = 0xff;
                buf[wlan2ChkAddr+1] = 0xff;
            }
#endif
#ifdef WLAN3_MAC_OFFSET
            if (mac_for == MAC_FOR_WIFI3) {
                buf[i/2+wlan3MacAddr] = mac[i-1]<<4 | mac[i];
                buf[wlan3ChkAddr] = 0xff;
                buf[wlan3ChkAddr+1] = 0xff;
            }
#endif
        }
    }
}

static int get_mac(int fd, char* interface)
{
    int offset = 0;
    /*
    *  LAN Interface
    */
#ifdef ETH0_MAC_OFFSET
    if (strcasecmp(interface, "eth0") == 0)
    {
        offset = ETH0_MAC_OFFSET;
    }
#endif
#ifdef ETH1_MAC_OFFSET
    if (strcasecmp(interface, "eth1") == 0)
    {
        offset = ETH1_MAC_OFFSET;
    }
#endif
#ifdef ETH2_MAC_OFFSET
    if (strcasecmp(interface, "eth2") == 0)
    {
        offset = ETH2_MAC_OFFSET;
    }
#endif
#ifdef ETH3_MAC_OFFSET
    if (strcasecmp(interface, "eth3") == 0)
    {
        offset = ETH3_MAC_OFFSET;
    }
#endif
#ifdef ETH4_MAC_OFFSET
    if (strcasecmp(interface, "eth4") == 0)
    {
        offset = ETH4_MAC_OFFSET;
    }
#endif
#ifdef ETH5_MAC_OFFSET
    if (strcasecmp(interface, "eth5") == 0)
    {
        offset = ETH5_MAC_OFFSET;
    }
#endif

    /*
    *  WLAN Interface
    */
#ifdef WLAN0_MAC_OFFSET
    if (strcasecmp(interface, "wifi0") == 0)
    {
        offset = WLAN0_MAC_OFFSET;
    }
#endif
#ifdef WLAN1_MAC_OFFSET
    if (strcasecmp(interface, "wifi1") == 0)
    {
        offset = WLAN1_MAC_OFFSET;
    }
#endif
#ifdef WLAN2_MAC_OFFSET
    if (strcasecmp(interface, "wifi2") == 0)
    {
        offset = WLAN2_MAC_OFFSET;
    }
#endif
#ifdef WLAN3_MAC_OFFSET
    if (strcasecmp(interface, "wifi3") == 0)
    {
        offset = WLAN3_MAC_OFFSET;
    }
#endif

    int buf_size = lseek(fd, 0, SEEK_END);
    printf("buf_size = %d bytes\n", buf_size);
    uint8_t* buf = (uint8_t*) malloc(sizeof(uint8_t) * buf_size);
    if (!buf)
    {
        printf("  ## %s() FAIL: cannot allocate memory for buf\n", __FUNCTION__);
        return EXIT_FAILURE;
    }

    if (offset != lseek(fd, offset, SEEK_SET))
    {
        printf("  ## %s() FAIL: lseek() error\n", __FUNCTION__);
        return EXIT_FAILURE;
    }

    int err = read(fd, buf, buf_size);
    if (err < 0)
    {
        printf("  ## %s() FAIL: read() error\n", __FUNCTION__);
        free(buf);
        return EXIT_FAILURE;
    }
    
    for (int i = 0; i < 6; ++i)
    {
        if (i)
        {
            printf(":");
        }
        printf("%02x", buf[i]);
    }
    printf("\n");

    free(buf);
    return EXIT_SUCCESS;
}

static int set_mac(int fd, char* interface, char* mac)
{
    int offset = 0;
    int mac_for = MAC_FOR_NONE;

    /*
    *  LAN Interface
    */
#ifdef ETH0_MAC_OFFSET
    if (strcasecmp(interface, "eth0") == 0)
    {
        mac_for = MAC_FOR_ETH0;
    }
#endif
#ifdef ETH1_MAC_OFFSET
    if (strcasecmp(interface, "eth1") == 0)
    {
        mac_for = MAC_FOR_ETH1;
    }
#endif
#ifdef ETH2_MAC_OFFSET
    if (strcasecmp(interface, "eth2") == 0)
    {
        mac_for = MAC_FOR_ETH2;
    }
#endif
#ifdef ETH3_MAC_OFFSET
    if (strcasecmp(interface, "eth3") == 0)
    {
        mac_for = MAC_FOR_ETH3;
    }
#endif
#ifdef ETH4_MAC_OFFSET
    if (strcasecmp(interface, "eth4") == 0)
    {
        mac_for = MAC_FOR_ETH4;
    }
#endif
#ifdef ETH5_MAC_OFFSET
    if (strcasecmp(interface, "eth5") == 0)
    {
        mac_for = MAC_FOR_ETH5;
    }
#endif

    /*
    *  WLAN Interface
    */
#ifdef WLAN0_MAC_OFFSET
    if (strcasecmp(interface, "wifi0") == 0)
    {
        mac_for = MAC_FOR_WIFI0;
    }
#endif
#ifdef WLAN1_MAC_OFFSET
    if (strcasecmp(interface, "wifi1") == 0)
    {
        mac_for = MAC_FOR_WIFI1;
    }
#endif
#ifdef WLAN2_MAC_OFFSET
    if (strcasecmp(interface, "wifi2") == 0)
    {
        mac_for = MAC_FOR_WIFI2;
    }
#endif
#ifdef WLAN3_MAC_OFFSET
    if (strcasecmp(interface, "wifi3") == 0)
    {
        mac_for = MAC_FOR_WIFI3;
    }
#endif

    int buf_size = lseek(fd, 0, SEEK_END);
    printf("buf_size = %d bytes\n", buf_size);
    uint8_t* buf = (uint8_t*) malloc(sizeof(uint8_t) * buf_size);
    if (!buf)
    {
        printf("  ## %s() FAIL: cannot allocate memory for buf\n", __FUNCTION__);
        return EXIT_FAILURE;
    }

    if (offset != lseek(fd, offset, SEEK_SET))
    {
        printf("  ## %s() FAIL: lseek() error\n", __FUNCTION__);
        return EXIT_FAILURE;
    }

    int err = read(fd, buf, buf_size);
    if (err < 0)
    {
        printf("  ## %s() FAIL: read() error\n", __FUNCTION__);
        free(buf);
        return EXIT_FAILURE;
    }

    printf("Read %d bytes from fd\n", err);

    mac_convert(buf, mac, mac_for);
    
#ifdef WLAN0_MAC_OFFSET
    if (mac_for == MAC_FOR_WIFI0)
    {
        int calcAddr = WLAN0_CHK_CALC;
        buf[WLAN0_NVMACFLAG_OFFSET]=1;
        buf[WLAN0_NUMMACADDR_OFFSET]=2;
        cal_checksum(buf, calcAddr, mac_for);
    }
#endif
#ifdef WLAN1_MAC_OFFSET
    if(mac_for == MAC_FOR_WIFI1)
    {
        int calcAddr = WLAN1_CHK_CALC;
        buf[WLAN1_NVMACFLAG_OFFSET]=1;
        buf[WLAN1_NUMMACADDR_OFFSET]=2;
        cal_checksum(buf, calcAddr, mac_for);

    }
#endif
#ifdef WLAN2_MAC_OFFSET
    if(mac_for == MAC_FOR_WIFI2)
    {
        int calcAddr = WLAN2_CHK_CALC;
        buf[WLAN2_NVMACFLAG_OFFSET]=1;
        buf[WLAN2_NUMMACADDR_OFFSET]=3;
        cal_checksum(buf, calcAddr, mac_for);
    }
#endif
#ifdef WLAN3_MAC_OFFSET
    if(mac_for == MAC_FOR_WIFI3)
    {
        int calcAddr=WLAN3_CHK_CALC;
        buf[WLAN3_NVMACFLAG_OFFSET]=1;
        buf[WLAN3_NUMMACADDR_OFFSET]=4;
        cal_checksum(buf, calcAddr, mac_for);
    }
#endif

    int fd2 = is_emmc_only ? find_mmc_art() : find_mtd_art();
    if (fd2 < 0)
    {
        printf("  ## %s() FAIL: Cannot get ART partition\n", __FUNCTION__);
        free(buf);
        return EXIT_FAILURE;
    }

    /*
    *   Erase data first
    */
    if (!is_emmc_only)
    {
        struct mtd_info_user mtd;
        err = getmeminfo(fd2, &mtd);
        if (err < 0)
        {
            perror ("MEMGETINFO");
            close(fd2);
            return EXIT_FAILURE;
        }
        erase_flash(fd2, 0, max(buf_size, mtd.erasesize));
    }

    err = write(fd2, buf, buf_size);
    close(fd2);

    if (err < 0)
    {
        printf("  ## %s() FAIL: write() error\n", __FUNCTION__);
        free(buf);
        return EXIT_FAILURE;
    }

    printf("Write %d bytes to fd2\n", err);

    if (mac_for == MAC_FOR_WIFI0)
        printf("The wifi0 MAC is set to %x%x%x%x%x%x%x%x%x%x%x%x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
    if (mac_for == MAC_FOR_WIFI1)
        printf("The wifi1 MAC is set to %x%x%x%x%x%x%x%x%x%x%x%x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
    if (mac_for == MAC_FOR_WIFI2)
        printf("The wifi2 MAC is set to %x%x%x%x%x%x%x%x%x%x%x%x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
    if (mac_for == MAC_FOR_WIFI3)
        printf("The wifi3 MAC is set to %x%x%x%x%x%x%x%x%x%x%x%x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
    if (mac_for == MAC_FOR_ETH0)
        printf("The eth0 MAC is set to %x%x%x%x%x%x%x%x%x%x%x%x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
    if (mac_for == MAC_FOR_ETH1)
        printf("The eth1 MAC is set to %x%x%x%x%x%x%x%x%x%x%x%x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
    if (mac_for == MAC_FOR_ETH2)
        printf("The eth2 MAC is set to %x%x%x%x%x%x%x%x%x%x%x%x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
    if (mac_for == MAC_FOR_ETH3)
        printf("The eth3 MAC is set to %x%x%x%x%x%x%x%x%x%x%x%x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
    if (mac_for == MAC_FOR_ETH4)
        printf("The eth4 MAC is set to %x%x%x%x%x%x%x%x%x%x%x%x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
    if (mac_for == MAC_FOR_ETH5)
        printf("The eth5 MAC is set to %x%x%x%x%x%x%x%x%x%x%x%x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);

    free(buf);
    return EXIT_SUCCESS;
}

/**
 * Process Get Operation
 */
static int do_get(int is_emmc_only, char* interface)
{
    int fd = is_emmc_only ? find_mmc_art() : find_mtd_art();
    if (fd < 0)
    {
        printf("  ## %s() FAIL: Cannot get ART partition\n", __FUNCTION__);
        return EXIT_FAILURE;
    }

    int err = get_mac(fd, interface);

    close(fd);
    return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

/**
 * Process Set Operation
 */
static int do_set(int is_emmc_only, char* interface, char* mac_address)
{
    int fd = is_emmc_only ? find_mmc_art() : find_mtd_art();
    if (fd < 0)
    {
        printf("  ## %s() FAIL: Cannot get ART partition\n", __FUNCTION__);
        return EXIT_FAILURE;
    }

    int err = set_mac(fd, interface, mac_address);

    close(fd);
    return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    int opt = 0;
    int is_get = 0;
    int is_set = 0;
    char* interface = NULL;
    char* mac_address = NULL;

    if (argc == 1)
    {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    while ((opt = getopt(argc, argv, "ehgi:s:")) != -1)
    {
        switch (opt) 
        {
            case 'e':
                is_emmc_only = 1;
                printf("ART partition in eMMC\n");
                break;

            case 'i':
                interface = (char*) malloc(sizeof(char) * (strlen(optarg) + 1));
                memcpy(interface, optarg, sizeof(char) * (strlen(optarg) + 1));
                printf("Interface = %s\n", interface);
                break;

            case 'g':
                is_get = 1;
                break;

            case 's':
                is_set = 1;
                mac_address = (char*) malloc(sizeof(char) * (strlen(optarg) + 1));
                memcpy(mac_address, optarg, sizeof(char) * (strlen(optarg) + 1));
                printf("MAC address = %s\n", mac_address);
                break;

            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;

            /* Error handle: Mainly missing arg or illegal option */
            case '?':
                // printf("%s: Illegal option: -%c\n", __FUNCTION__, isprint(optopt) ? optopt : '#');
                break;

            default:
                printf("%s: Too few argument!\n", __FUNCTION__);
                break;
        }
    }

    if (verify_option(is_get, is_set, interface, mac_address))
    {
        return EXIT_FAILURE;
    }

    int op_code = is_get ? OP_GET : is_set ? OP_SET : OP_NONE;
    int err = 0;

    switch (op_code)
    {
        case OP_GET:
            printf("Operation Get.\n");
            err = do_get(is_emmc_only, interface);
            break;
        
        case OP_SET:
            printf("Operation Set.\n");
            err = do_set(is_emmc_only, interface, mac_address);
            break;

        case OP_NONE:
        default:
            break;
    }

    if (interface)
    {
        free(interface);
    }
    if (mac_address)
    {
        free(mac_address);
    }

    return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
