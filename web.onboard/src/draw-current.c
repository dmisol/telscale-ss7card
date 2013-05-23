#include <stdio.h>
#include <string.h>
#include "cgi-current.h"
#include "sgw.h"
#include "../../portnumbers.h"

#define PATH	"/bin/"

int display_sgw(int index){
	FILE *f;
	char *pos, str[500];
	
	if(index) sprintf(str,PATH"m3ua%d.conf",index);
	else sprintf(str,PATH"m3ua.conf");
	
	f = fopen(str,"r");
	if(!f){
		return 0;
	}
	
	fgets(str,sizeof(str),f);
	
	sgw[index].opc=0;
	sgw[index].dpc=0;
	sgw[index].ni=0;
	sgw[index].links=1;
	sgw[index].linkid = 0;
	sgw[index].appid = 0;
	sgw[index].tcp = 0;
	sgw[index].sctp = 0;
	sgw[index].first_sls = 0;
	strcpy(sgw[index].ip,"");
	sgw[index].port = 0;
	

	if((pos=strstr(str,"linkid="))) { sscanf(pos+7,"%d",&sgw[index].linkid); }	 // shm key for the first link
	if((pos=strstr(str,"appid="))) { sscanf(pos+6,"%d",&sgw[index].appid); }	   // shm key for logging

	if((pos=strstr(str,"links="))) { sscanf(pos+6,"%d",&sgw[index].links); }	   // no of SS7 links
	
	if((pos=strstr(str,"tcp="))) { sscanf(pos+4,"%d",&sgw[index].tcp); }		 // tcp server port
	if((pos=strstr(str,"sctp="))) { sscanf(pos+5,"%d",&sgw[index].sctp); }	     // sctp server port
	
	if((pos=strstr(str,"opc="))) { sscanf(pos+4,"%d",&sgw[index].opc); }		 // own SPC
	if((pos=strstr(str,"dpc="))) { sscanf(pos+4,"%d",&sgw[index].dpc); }		 // another end of the link
	if((pos=strstr(str,"sls="))) { sscanf(pos+4,"%d",&sgw[index].first_sls); }		 // another end of the link
	if((pos=strstr(str,"ni=")))  {						    // ni value
	sscanf(pos+3,"%d",&sgw[index].ni); 
	switch(sgw[index].ni){ 	case 10: sgw[index].ni = 2; break;
				case 11: sgw[index].ni = 3; break;}}	
	
	if((!sgw[index].tcp) && (!sgw[index].sctp)) sgw[index].sctp = DEFAULT_M3UA_PORT;

	pos = strstr(str," ");
	if(pos) sscanf(pos+1,"%s %d",&sgw[index].ip[0],&sgw[index].port);
	
	printf("\n<tr><td class=\'sgw%d\'>",index);
	if(sgw[index].tcp)	printf("M3UA/TCP</td><td>%d</td><td>",sgw[index].tcp);
	else			printf("M3UA/SCTP</td><td>%d</td><td>",sgw[index].sctp);
	
	printf("- </td><td>%d</td><td>",sgw[index].links);
	printf("%d</td><td>%d</td><td>",sgw[index].dpc,sgw[index].opc);
	printf("%d</td><td>",sgw[index].ni);
	if(sgw[index].port)	printf("%s:%d</td></tr\n>",sgw[index].ip,sgw[index].port);
	else			printf("</td></tr\n>");
	
	fclose(f);
	return 1;
}
int linknum;
int get_linkset(int i){
	int sum = 0;
	
	for(int s=0;s<4;s++, sum+=sgw[s].links)
		if((i>=sgw[s].linkid) && (i<(sgw[s].linkid + sgw[s].links))){
		    linknum = i-sgw[s].linkid;
		    return s;
		}
	return 0;
}
void display_hw(int index){
	FILE *f;
	char *pos, str[500];
	int hdlc[64];
	int ts[32];
	int qty,ind,val;
	
	if(index) sprintf(str,PATH"hdlc%d.conf",index);
	else sprintf(str, PATH"hdlc.conf");
	
	if(!(f = fopen(str,"r"))){
		return;
	}
	
	fgets(str,sizeof(str),f);
	
	pos = strstr(str," ");
	sscanf(pos,"%d",&qty);
	pos++;
	
	//printf("\n\n%d links\n\n",qty);
	
	for(int i=0;i<32;i++){
		hdlc[i] = hdlc[i+32] = -1;
		ts[i] = -1;
	}
	
	for(int i=0;i<qty;i++){
		pos = strstr(pos," ");pos++;
		sscanf(pos,"%d:%d",&ind,&val);
		ts[ind] = val;
		hdlc[val] = ind;
	}
	printf("\n<tr><td>%d</td>",index);
	for(int i=1;i<32;i++)
		if(ts[i] == -1) printf("<td></td>");
		else	{
			int ls = get_linkset(ts[i]);
			printf("<td class=\'sgw%d\'>%02d</td>",ls,linknum);
		}
	printf("</tr>\n");

}
main(){
	int i=0;
	
	printf("%s",hdr);
	
	while(display_sgw(i++));
	
	printf("%s",mid);
	
	display_hw(0);
	display_hw(1);	
	
	printf("%s\n\n",btm);
}