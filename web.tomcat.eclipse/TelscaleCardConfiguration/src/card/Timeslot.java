/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package card;

import session.Configuration;

/**
 *
 * @author dmisol
 */
public class Timeslot {
	protected int ts;
	protected Sgw sgw;
	protected int pcmId;
	
	protected String title;
	
	public Timeslot(int pcm, int tslot){
	    this.pcmId = pcm;
	    this.ts = tslot;
	    this.sgw = null;
	    
	    title = "timeslot № " + ts;
	}
	
	void setSgw(Sgw gw){
	    sgw = gw;
	    if(sgw == null) title = "timeslot № " + ts;
	}
	
	public Sgw getSgw(){
	    return sgw;
	}
	
	public int getPcmId(){
	    return pcmId;
	}
	
	private int getSgwId(){
	    if(sgw == null) return -1;
	    return sgw.getSls(this);
	}
		
	public int getGlobalLinkId(){
	    if(sgw!=null) return sgw.getGlobalLinkId(this);
	    else return -1;
	}
	
	
	
	
public String sgwColor(){
	if(sgw == null) return "empty";
	return sgw.getColor();
}
	
public String isLocked(){
	if(ts==0) return "true";
	if(sgw == null) return "false";
	
	if(Configuration.isCurrentSgw(sgw))
		return "false";
	else	return "true";
}

public String toggleStatus(){
	if(sgw == null) {
		if(Configuration.assignTimeslot(this))
			title = "timeslot № " + ts + ", Gateway № " + sgw.getSgwId() + ", SLS № " + sgw.getLinkId(this);
	}
	else {
		Configuration.freeTimeslot(this);
		title = "timeslot № " + ts;
	}
	return "";
}
public String getTitle(){
	return title;
}

public String buttonSignature(){
	if(Configuration.displayOnly()){
	    if(ts==0) return "PCM" + this.pcmId;
	    if(sgw==null) return "XX";

	int sls = sgw.getSls(this);
	if(sls>9)	return ""+sls;
	else		return "0"+sls;
	}

	if(ts==0) return "PCM" + this.pcmId;

	if(isLocked().equals("true")) return "XX";
	if(sgw == null){
		if(ts>9) return ""+ts;
		else return "0"+ts;
	}
	else	{
		int sls = sgw.getSls(this);
		if(sls>9)	return ""+sls;
		else		return "0"+sls;		
	}
}

}
