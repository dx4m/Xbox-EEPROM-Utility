var defaultMode = true;

function getInfo(){
    var xhttp = new XMLHttpRequest();
    
    var hdkey = document.getElementById("hdkey");
    var serial = document.getElementById("serial");
    var xversion = document.getElementById("xversion");
    var macaddr = document.getElementById("macaddr");
    
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var xjson = JSON.parse(this.responseText);
            hdkey.style.color = "black";
            hdkey.innerHTML = xjson.hdkey;
            serial.innerHTML = xjson.serial;
            xversion.innerHTML = xjson.xversion;
            macaddr.innerHTML = xjson.macaddr;
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
        exit();
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
        exit();
    }
    
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "/", true);
    
    xhr.onload = function(){
        if(this.status == 200){
            document.getElementById("flashsuccess").innerHTML = "flash was " + xhr.responseText;
            getInfo();
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

getInfo();

