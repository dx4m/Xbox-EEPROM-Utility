var defaultMode = true;
var isConsoleOnFlag = 0;

function togglePower(isEject) {
    var xhttp = new XMLHttpRequest();
    if(isEject == 1){
    	xhttp.open("GET", "/xwifi?eject", false);
    	xhttp.send();
    }
    else{
    	xhttp.open("GET", "/xwifi?pwr", false);
    	xhttp.send();
    }
}

function isConsoleOn(){
	var xhttp = new XMLHttpRequest();
	var retval;
	xhttp.onreadystatechange = function(){
		if(this.readyState == 4 && this.status == 200){
			if(this.responseText == "YES"){
				isConsoleOnFlag = 1;
			}
			
			if(this.responseText == "NO"){
				isConsoleOnFlag = 0;
			}
		}
	}
	xhttp.open("GET", "/xwifi?ison", true);
	xhttp.send();
}

function getInfo(k){
    var xhttp = new XMLHttpRequest();
    
    var hdkey = document.getElementById("hdkey");
    var serial = document.getElementById("serial");
    var xversion = document.getElementById("xversion");
    var macaddr = document.getElementById("macaddr");
    
    if(!k){
    	isConsoleOn(); // Sets or clears the isConsoleOnFlag
    }
    
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var xjson = JSON.parse(this.responseText);
            if(isConsoleOnFlag){
            	hdkey.style.color = "black";
            	serial.style.color = "black";
            	hdkey.innerHTML = xjson.hdkey;
            	serial.innerHTML = xjson.serial;
            	xversion.innerHTML = xjson.xversion;
            	macaddr.innerHTML = xjson.macaddr;
            }
            else{
            	hdkey.style.color = "red";
            	serial.style.color = "red";
            	hdkey.innerHTML = "Your Xbox is off.";
            	serial.innerHTML = "Please turn on your Xbox";
            	xversion.innerHTML = "";
            	macaddr.innerHTML = "";
            }
        }
    };
    xhttp.open("GET", "/upd", true);
    xhttp.send();
}

function changeKey(){
    var key = document.getElementById("newHDkey").value;
    //everything else
    
    if(key.length < 32){
        alert("The key must have 32 characters!");
        return;
    }
    isConsoleOn();
    if(isConsoleOnFlag == 0){
    	alert("Please turn on your Xbox!");
    	return;
    }
    
    var hdHide = document.getElementById("hdkey");
    //hdHide.style.color = "red";
    
    var postKey = new XMLHttpRequest();
    
    postKey.open("GET", "/upd?hdkey=" + key, true);
    postKey.onreadystatechange = function(){
        if(postKey.readyState == 4 && postKey.status == 200){
            var xjson = JSON.parse(this.responseText);
            hdHide.style.color = "green";
            hdHide.innerHTML = xjson.hdkey;
        }
        else if(postKey.status != 200) {
            hdHide.style.color = "red";
        }
    }
    
    postKey.send();
}

function flashEEPROM(){
    
    if(document.eepromform.eeprom.value == ""){
        alert("Please, select first a file.");
        return;
    }
    
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "/", true);
    
    xhr.onload = function(){
        if(this.status == 200){
            document.getElementById("flashsuccess").innerHTML = "flash was " + xhr.responseText;
            getInfo(0);
        }
        else{
            document.getElementById("flashsuccess").innerHTML = "flash was " + xhr.responseText;
            document.getElementById("flashsuccess").style.color = "red";
        }
        
    };
    var formData = new FormData(document.getElementById("eepromfile"));
    xhr.send(formData);
}

function setReset(){
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/smc?reset", false);
    xhttp.send();
}

function nullKey(){
    document.getElementById("newHDkey").value = "00000000000000000000000000000000";
}

function clearKey(){
    document.getElementById("newHDkey").value = "";
}

function copyKey(){
    var hdkey = document.getElementById("hdkey");
    var range = document.createRange();
    range.selectNode(hdkey);
    window.getSelection().addRange(range);
    
    document.execCommand("copy");
    alert("Copied " + hdkey.innerHTML + " successfully.");
}

function backupEEPROMToFlash(){
    var xhttp = new XMLHttpRequest();
    xhttp.onload = function(){
        if(this.status == 200){
            if(this.responseText == "OK"){
            	alert("Backup was a success!");
            }
            else if(this.responseText == "FAILED")
            {
            	alert("Backup failed. Probably because there is already a backup?");
            }
        }
        else{
            alert("Something went wrong.");
        }
        
    };
    xhttp.open("GET", "/xwifi?bakeep", true);
    xhttp.send();
}

function restoreEEPROMToFlash(){
    var xhttp = new XMLHttpRequest();
    xhttp.onload = function(){
        if(this.status == 200){
            if(this.responseText == "OK"){
            	alert("Restore was a success!");
            }
            else if(this.responseText == "FAILED")
            {
            	alert("Restore failed.");
            }
        }
        else{
            alert("Something went wrong.");
        }
        
    };
    xhttp.open("GET", "/xwifi?resteep", true);
    xhttp.send();
}

function loadConfiguration(){
    var standby_checkbox = document.getElementById("standby");
    var bootfade_checkbox = document.getElementById("bootfade");
    var redLedInput = document.getElementById("redled");
    var greenLedInput = document.getElementById("greenled");
    //everything else
    
    var xhttp = new XMLHttpRequest();
    
    xhttp.open("GET", "/config?load_config", true);
    xhttp.onreadystatechange = function(){
        if(xhttp.readyState == 4 && xhttp.status == 200){
            var xjson = JSON.parse(this.responseText);
            standby_checkbox.checked = xjson.standby_light ? true : false;
            redLedInput.value = xjson.standby_red_val;
            greenLedInput.value = xjson.standby_green_val;
            bootfade_checkbox.checked = xjson.boot_led_fade ? true : false;
            
        }
    }
    
    xhttp.send();
}

function saveConfiguration(){
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/config?save_config", false);
    xhttp.send();
}

function resetConfiguration(){
    var delInternalBackup = 0;
    var delConfiguration = 0;
    if(confirm("WARNING: Are you sure about to \"FACTORY\" reset your configuration?")){
    	delConfiguration = 1;
    }
    if(confirm("Delete EEPROM backup(when there is one made) of your Xbox from the internal flash?")){
    	delInternalBackup = 1;
    }
    
    if(delInternalBackup){
    	var xhttp = new XMLHttpRequest();
    	xhttp.open("GET", "/config?delBackup", false);
    	xhttp.send();
    }
    if(delConfiguration){
    	var xhttp = new XMLHttpRequest();
    	xhttp.open("GET", "/config?factory", false);
    	xhttp.send();
    	
    }
}

function setStandbyLight(){
    var standby_checkbox_checked = document.getElementById("standby").checked ? 1 : 0;
    
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/config?standby_light=" + standby_checkbox_checked, false);
    xhttp.send();
}

function setBootLEDFade(){
    var bootledfade_checkbox_checked = document.getElementById("bootfade").checked ? 1 : 0;
    
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/config?boot_led_fade=" + bootledfade_checkbox_checked, false);
    xhttp.send();
}

function setStandbyGreenValue(){
    var standby_green_val = document.getElementById("greenled").value;
    
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/config?standby_green_val=" + standby_green_val, false);
    xhttp.send();
}

function setStandbyRedValue(){
    var standby_red_val = document.getElementById("redled").value;
    
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/config?standby_red_val=" + standby_red_val, false);
    xhttp.send();
}

function resetESP(){
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/config?reset", false);
    xhttp.send();
}

function changeTo(){
    var link = document.getElementById("downloadEEPROM");
    var mode = document.getElementById("decryptedMode");
    if(defaultMode == true){
        link.innerHTML = "!!WARNING!! Download your DECRYPTED decr_eeprom.bin";
        link.href = "/decr_eeprom.bin";
        mode.style.visibility = "hidden";
        defaultMode = false
    }
    else{
        link.innerHTML = "Download your eeprom.bin";
        link.href = "/eeprom.bin";
        mode.style.visibility = "visible";
        defaultMode = true;
    }
}

function updateESP(){
    
    if(document.updateform.update.value == ""){
        alert("Please, select first a file.");
        return;
    }
    
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "/update", true);
    var formData = new FormData(document.getElementById("updatefile"));
    xhr.send(formData);
    alert("Update completed. Wait a few seconds, so your ESP can reset correctly and then press OK. :)");
    location.reload(true);
}

var actualUploadButton = document.getElementById("eepromfilename");
var fileChose = document.getElementById("filechosen");

actualUploadButton.addEventListener('change', function(){
  fileChose.textContent = this.files[0].name
});

var updateUploadButton = document.getElementById("updatefilename");

updateUploadButton.addEventListener('change', function(){
  updateESP();
});

getInfo(0);
loadConfiguration();
const interval = setInterval(getInfo, 5000, 0);

