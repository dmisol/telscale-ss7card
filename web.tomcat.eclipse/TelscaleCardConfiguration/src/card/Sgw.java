/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package card;

import java.util.ArrayList;
import java.util.List;
import session.Configuration;

/**
 *
 * @author dmisol
 */
public class Sgw {
    private static final int MaxSpcValue = 16383;
    
    private int appId;
    private int firstLink;
    
    private String name;
    private String transport;
    private int port;
    private int opc;
    private int dpc;
    private int ni;
    private int linkSetSize = 0;

    private String errorMsg = new String();
    
    protected List<Timeslot> links = new ArrayList<Timeslot>();

    public Sgw(int sgwId){//int appid, int firstLink, String name, String transport, int port, int opc, int dpc, int ni) {
	appId = sgwId;
	this.firstLink = firstLink;
	linkSetSize = 0;
	name = "m3ua";
	transport = "sctp";
	port = 2905 + appId*10;
	opc = 0;
	dpc = 0;
	ni = 2;
	
    }

    public int delete(){
	int counter = links.size();
	while(! links.isEmpty()) {
	    
	    removeLink(links.get(links.size()-1));
	}
	return counter;
    }
    public int getSgwId(){
	return appId;
    }

    public boolean addLink(Timeslot ts){
	if(links.size()<16){
		links.add(ts);
		ts.setSgw(this);
		linkSetSize = links.size();
		return true;
	}
	Configuration.setErrorMessage("a linkset may have up to 16 links");
	return false;
    }

    public void removeLink(Timeslot ts){
	ts.setSgw(null);
	links.remove(ts);
	linkSetSize = links.size();
    }

    public int getSls(Timeslot ts){
	return links.indexOf(ts);
    }

    public int getGlobalLinkId(Timeslot ts){
	int ind = links.indexOf(ts);
	if(ind >= 0) return firstLink + ind;
	return -1;
    }


    public boolean isOk(){
	errorMsg = new String();
	if(((opc<=0)||(dpc<=0))||((opc>MaxSpcValue)||(dpc>MaxSpcValue))){
	    errorMsg = "invalid SPC value";
	    return false;
	}
	if(opc == dpc){
	    errorMsg = "OPC and DPC values must differ";
	    return false;
	}
	if(links.isEmpty()) {
	    errorMsg = " no links assigned";
	    return false;
	}

	return true;
    }
    public String verify(){
	if(isOk())  return "gateway looks fine";
	else return errorMsg;
    }
    public String verifyStyle(){
	if(isOk()) return "gateways";
	return "error";
    }

    public int getLinkSetSize(){
	return linkSetSize;
    }

    public int getOpc(){
	return opc;
    }
    public void setOpc(int v){
	opc = v;
    }
    public int getDpc(){
	return dpc;
    }
    public void setDpc(int v){
	dpc = v;
    }

    public int getNi(){
	return ni;
    }
    public void setNi(int v){
	if((v>0)&&(v<4)) ni = v;
	if((v==10)||(v==11)) ni = v;
    }

    public int getPort(){
	return port;
    }
    public void setPort(int p){
	port = p;
    }

    public void setTransport(String v){
	String lower = v.toLowerCase();
	if(lower.equals("tcp")) transport = v;
	transport = "sctp";
    }
	
    public String getTransport(){
	if(transport.isEmpty()) return "sctp";
	return transport;
    }

    public String conf(){
	if(appId>0)
		return	"appid=" + appId+
			",linkid=" + firstLink+
			",links=" + getLinkSetSize()+
			",opc=" + getOpc() +
			",dpc=" + getDpc() +
			",ni=" + getNi() +
			"," + getTransport() +"="+port;
	else	return	"links=" + getLinkSetSize()+
			",opc=" + getOpc() +
			",dpc=" + getDpc() +
			",ni=" + getNi() +
			"," + getTransport() +"="+port;
    }

    public String getName(){
	return name;
    }

    public int getLinkId(Timeslot ts){
	return links.indexOf(ts);	
    }




    public String getColor(){
	return "sgw" + appId;
    }
    public String linksNumerStyle(){
	if(links.size()==0) return "error";
	return "gateways";
    }
}
