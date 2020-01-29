function getKey(){
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("currentHDkey").innerHTML =
            this.responseText;
            document.getElementById("currentHDkey").style.color = "blue";
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
    
    var hdHide = document.getElementById("currentHDkey");
    //hdHide.style.color = "red";
    
    var postKey = new XMLHttpRequest();
    
    postKey.open("GET", "/upd?hdkey=" + key, true);
    postKey.onreadystatechange = function(){
        if(postKey.readyState == 4 && postKey.status == 200){
            hdHide.style.color = "green";
            hdHide.innerHTML = postKey.responseText;
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
            getKey();
            document.getElementById("currentHDkey").style.color = "blue";
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

getKey();
