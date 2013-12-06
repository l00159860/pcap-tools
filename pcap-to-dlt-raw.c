#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pcap.h>
#include <err.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>

#include "pcap_layers.h"

const char *progname;
struct pcap_pkthdr out_hdr;
const u_char *out_data;

/*
 * this will only be called if 'ip' is a complete IPv header
 */
int
my_ip4_handler(const struct ip *ip4, int len, void *userdata)
{
    out_hdr.caplen = len;
    out_data = (void*) ip4;
    return 0;
}

int
my_ip6_handler(const struct ip6_hdr *ip6, int len, void *userdata)
{
    out_hdr.caplen = len;
    out_data = (void*) ip6;
    return 0;
}

int
main(int argc, char *argv[])
{
    pcap_t *in = NULL;
    char errbuf[PCAP_ERRBUF_SIZE + 1];
    struct pcap_pkthdr in_hdr;
    const u_char *data;
    pcap_dumper_t *out = NULL;
    pcap_t *dead;

    progname = argv[0];
    if (argc < 2) {
	fprintf(stderr, "usage: %s pcapfiles\n", progname);
	exit(1);
    }

    in = pcap_open_offline(argv[1], errbuf);
    if (NULL == in) {
	fprintf(stderr, "%s: %s", argv[1], errbuf);
	exit(1);
    }
    dead = pcap_open_dead(DLT_RAW, 65536);
    if (NULL == dead) {
	perror("pcap_open_dead");
	exit(1);
    }
    out = pcap_dump_open(dead, "-");
    if (NULL == out) {
	perror("stdout");
	exit(1);
    }
    pcap_layers_init(pcap_datalink(in), 0);
    callback_ipv4 = my_ip4_handler;
    callback_ipv6 = my_ip6_handler;
    while ((data = pcap_next(in, &in_hdr))) {
	    out_hdr.ts = in_hdr.ts;
	    out_hdr.caplen = 0;
	    out_data = NULL;
	    handle_pcap(NULL, &in_hdr, data);
	    if (out_data && out_hdr.caplen) {
	        out_hdr.len = in_hdr.len + out_hdr.caplen - in_hdr.caplen;
		pcap_dump((void *)out, &out_hdr, out_data);
	    }
    }
    pcap_close(in);
    pcap_dump_close(out);
    exit(0);
}

