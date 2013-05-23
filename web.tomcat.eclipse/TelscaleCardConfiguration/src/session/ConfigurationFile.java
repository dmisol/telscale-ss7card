/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package session;
import javax.faces.context.*;
import java.io.*;
/**
 *
 * @author dmisol
 */

public class ConfigurationFile {
	protected String name;
	protected String data;
	
	public ConfigurationFile(String v, String s){
		name = v;
		data = s;
	}
	
	public String getName(){
		return name;
	}

	public void download() throws IOException {

		FacesContext fc = FacesContext.getCurrentInstance();
		ExternalContext ec = fc.getExternalContext();

		ec.responseReset(); // Some JSF component library or some Filter might have set some headers in the buffer beforehand. We want to get rid of them, else it may collide.
		ec.setResponseContentType("text/HTML"); // Check http://www.w3schools.com/media/media_mimeref.asp for all types. Use if necessary ExternalContext#getMimeType() for auto-detection based on filename.
		ec.setResponseContentLength(data.length()); // Set it with the file size. This header is optional. It will work if it's omitted, but the download progress will be unknown.
		ec.setResponseHeader("Content-Disposition", "attachment; filename=\"" + name + "\""); // The Save As popup magic is done here. You can give it any file name you want, this only won't work in MSIE, it will use current request URL as file name instead.

		OutputStream output = ec.getResponseOutputStream();
		
		// Now you can write the InputStream of the file to the above OutputStream the usual way.
		output.write(data.getBytes());

		fc.responseComplete(); // Important! Otherwise JSF will attempt to render the response which obviously will fail since it's already written with a file and closed.
	}
}
