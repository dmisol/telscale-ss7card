/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package session;

import card.*;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import javax.faces.model.ArrayDataModel;
import javax.faces.model.DataModel;

/**
 *
 * @author dmisol
 */
public class Configuration {
	static final public int PcmQty = 2;
	static final public int AllowedSgws = 4;
	static final public int AllowedLinks = 34;
	
	static int linksAssigned;
	protected static List<Pcm> pcms = new ArrayList<Pcm>();
	protected static List<Sgw> sgws = new ArrayList<Sgw>();
	protected static List<ConfigurationFile> files = new ArrayList<ConfigurationFile>();
	
	protected static Sgw sgw;
	protected static int currentSgwId;

	protected static String errorMessage = new String();

public Configuration(){
	linksAssigned = 0;
	
	pcms.clear();
	for(int i=0;i<PcmQty;i++){
		Pcm p = new Pcm(i);
		pcms.add(p);
	}
	
	sgw = new Sgw(0);
	sgws.clear();
	addSgw();
	
	files.clear();
}

static public void setErrorMessage(String v){
	errorMessage = v;
}

public String getErrorMessage(){
	return errorMessage;
}

public String getHelpMessage(){
	return "help message";
}

static public boolean isCurrentSgw(Sgw s){
	return (s == sgw);
}
static public boolean assignTimeslot(Timeslot ts){
	if(linksAssigned == AllowedLinks) {
	    setErrorMessage("maximum number of links for a card is reached. another card is needed");
	    return false;
	}
	if( sgw.addLink(ts) ){
		linksAssigned++;
		return true;
	} 
	return false;
}

static public boolean displayOnly(){
	return (!files.isEmpty());
}
static public void freeTimeslot(Timeslot ts){
	sgw.removeLink(ts);
	linksAssigned--;
}

private void addSgw(){
	sgw = new Sgw(sgws.size());
	sgws.add(sgw);
	currentSgwId = sgws.size()-1;
}
private void removeSgw(){
	linksAssigned -= sgw.delete();
	sgws.remove(sgws.size()-1);
	sgw = sgws.get(sgws.size()-1);
	currentSgwId = sgws.size()-1;
}




public DataModel pcmDataModel(){
	return new ArrayDataModel(pcms.toArray());
}
	
public DataModel gatewaysDataModel(){
	return new ArrayDataModel(sgws.toArray());
}

public DataModel filesDataModel(){
	return new ArrayDataModel(files.toArray());
}



public String applySgwButtonAction(){
	setErrorMessage("");
	sgw.isOk();
	return "";
}

public String addSgwButtonDisabled(){
	if( sgw.isOk() &&(sgws.size()<AllowedSgws) ) return "false";
	return "true";
}

public String addSgwButtonAction(){
	setErrorMessage("");
	if(linksAssigned == AllowedLinks){
	    setErrorMessage("maximum number of links for a card is reached. no links can be assigned to a new gateway");
	    return "";
	}
	if( sgw.isOk() ) {
	    if(sgws.size()<(AllowedSgws)) addSgw();
	    return "newSgw";
	}
	return "";
}

public String removeSgwButtonDisabled(){
	if( sgws.size()<=1) return "true";
	return "false";
}

public String removeSgwButtonAction(){
	setErrorMessage("");
	if(! sgws.isEmpty()){
	    removeSgw();
	}
	return "newSgw";
}

public String fileSgwButtonDisabled(){
	for(Iterator iter = sgws.iterator(); iter.hasNext();) {
		Sgw sgw = (Sgw) iter.next();
		if(sgw.isOk()) continue;
		return "true";
	}return "false";
}

public String fileSgwButtonAction(){
	files.clear();
	ConfigurationFile f = new ConfigurationFile("hdlc.conf",pcms.get(0).conf());
	files.add(f);
	
	f = new ConfigurationFile("hdlc1.conf",pcms.get(1).conf());
	files.add(f);

	int ind = 0;
	for(Iterator iter = sgws.iterator(); iter.hasNext();) {
		Sgw sgw = (Sgw) iter.next();
		if(sgw.getSgwId()==0) 
			f = new ConfigurationFile("m3ua.conf",sgw.conf());
		else 
			f = new ConfigurationFile("m3ua" + sgw.getSgwId() + ".conf",sgw.conf());
		files.add(f);
	}
	return "saveFiles";
}

public String edit(){
	files.clear();
	return "newSgw";
}
}

