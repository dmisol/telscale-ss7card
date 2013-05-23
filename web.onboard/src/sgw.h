#define MAX_SGW	5
struct SGW{
    int opc;
    int dpc;
    int ni;
    int links;
    int linkid;
    int appid;
    int tcp;
    int sctp;
    
    int first_sls;
    char ip[50];
    int port;	} sgw[MAX_SGW];

