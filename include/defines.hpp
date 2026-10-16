#ifndef DEFINES_HPP
#define DEFINES_HPP

#include <linux/if_ether.h>
#include <netinet/ip.h>

#define IP_LEN sizeof(in_addr)
#define UPF_BUF_SIZE (IP_MAXPACKET + ETH_HLEN)

#endif // DEFINES_HPP
