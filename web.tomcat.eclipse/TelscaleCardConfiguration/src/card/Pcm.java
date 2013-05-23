/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package card;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import javax.faces.model.ArrayDataModel;
import javax.faces.model.DataModel;

/**
 *
 * @author dmisol
 */
public class Pcm {
    protected int pcmId;
    protected List<Timeslot> ts = new ArrayList<Timeslot>();
    
    
    public Pcm(int id){
	pcmId = id;
	int i;
	for(i=0;i<32;i++){
	    Timeslot t = new Timeslot(id,i);
	    ts.add(t);
	}
    }
    public void setPcmId(int v){
	pcmId = v;
    }
    
    public Timeslot getTsByIndex(int i){
	if((i>0)&&(i<32)) return ts.get(i);
	return null;
    }
    
    private int countLinks(){
	int res = 0;
	for(Iterator iter = ts.iterator(); iter.hasNext();) {
		Timeslot t = (Timeslot) iter.next();
		if(t.getGlobalLinkId()>=0) res++;
	}
	return res;
    }
    
    public String conf(){
	int qty = countLinks();
	String result = "/dev/mysport" + pcmId + " " + qty;
	
	int pos = 0;
	for(Iterator iter = ts.iterator(); iter.hasNext(); pos++) {
		Timeslot t = (Timeslot) iter.next();
		int id = t.getGlobalLinkId();
		if(id>=0) result += " " + pos + ":" + id;
	}
	return result;
    }





    public DataModel tsDataModel(){
	return new ArrayDataModel(ts.toArray());
    }
}
