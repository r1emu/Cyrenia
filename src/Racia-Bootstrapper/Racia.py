#!/usr/bin/env python

import sys, os;
import requests;
import shutil;
from lxml import etree;
from threading import Thread;
import re;
import SimpleHTTPServer;
import SocketServer;
from signal import *;

def http_server():
    server = SocketServer.TCPServer(("", 80), SimpleHTTPServer.SimpleHTTPRequestHandler)
    server.handle_request()

def signal_handler(s, frame):
    # Restore the backup
    shutil.copy(clientXmlPath + ".backup", clientXmlPath);
    print "Everything has been restored ! :)";
    sys.exit(0);

if __name__ == '__main__':

    if (len(sys.argv) != 2):
        print "Usage : %s <Client folder>" % os.path.basename(sys.argv[0]);
        sys.exit(0);

    # Read the server list URL in client.xml
    clientXmlPath = sys.argv[1] + "/release/client.xml";
    with open(clientXmlPath, 'rb') as clientXmlFile:
        clientXml = etree.fromstring(clientXmlFile.read());

    # Get the ServerListURL within the XML
    serverListUrl = clientXml.find("GameOption").get("ServerListURL");

    # Retrieve the server list from the URL
    print "Retrieving the server list at '%s' ... " % serverListUrl;
    serverListXml = etree.fromstring(requests.get(serverListUrl).content);

    # Get the server list from the XML
    barracksArray = serverListXml.findall("server");
    barracks = [[] for x in barracksArray]; # Build an array
	
    srvCount = 0;
    for id, barrack in enumerate(barracksArray):
        srvCount += 1;
        serverCount = 0;
        while (barrack.attrib.has_key("Server%d_IP" % serverCount)):
            ip = "Server%d_IP" % serverCount;
            port = "Server%d_Port" % serverCount;
            barracks[id].append({
                'ip' : barrack.attrib[ip], 
                'port' : barrack.attrib[port],
                'xmlNode' : barrack
            });
            serverCount += 1;

    # Print server list
    print "================ Server list : ================"

    for id, server in enumerate(barracks):
        print "%d : " % id;
        for barrack in server:
            print "\t %s:%s" % (barrack["ip"], barrack["port"]);
    print "================================================"
	
    # Ask for the server ID
    serverIdUser = raw_input("\nEnter the server ID you wish to listen to : ");

    # Check for the user input
    if (serverIdUser.isdigit() == False): # Must be a digit
        print "Please enter a digit.";
        sys.exit(0);

    serverIdUser = int(serverIdUser);
    if (serverIdUser < 0 or serverIdUser > srvCount): # Must be between 0 and the max server count
        print "Please enter a digit between 0 and %d." % srvCount;
        sys.exit(0);

    # Change the barracks Name in the XML
    barrackXmlNode = barracks[serverIdUser][0]["xmlNode"];
    barrackXmlNode.attrib["NAME"] = "(Proxy)";

    # Change the barracks IP in the XML
    for attribute in barrackXmlNode.attrib:
        serverIdMatch = re.compile("Server[0-9]+_").match(attribute);
        if (serverIdMatch != None):
            # Remove all the ServerX_ attributes
            del barrackXmlNode.attrib[attribute];
    # Add only one IP/Port
    barrackXmlNode.set("Server0_IP", "127.0.0.1");
    barrackXmlNode.set("Server0_Port", "7001");

    # Write the serverlist into a file for the HTTP server
    with open('serverlist.xml', 'wb+') as serverListXmlFile:
        serverListXmlFile.write(etree.tostring(serverListXml) + "\n");

    # Make a backup copy of client.xml
    shutil.copy(clientXmlPath, clientXmlPath + ".backup");
    clientXml.find("GameOption").set("ServerListURL", "http://127.0.0.1/serverlist.xml");

    # Replace the client.xml with our custom one
    with open(clientXmlPath, 'wb+') as clientXmlFile:
        clientXmlFile.write(etree.tostring(clientXml));

    print "You can now launch your client as usual !";
    
    signal(SIGINT, signal_handler);

    try:
        print ("Waiting for the client to connect and exit ...\n");
        thread = Thread(target = http_server);
        thread.setDaemon(True);
        thread.start();
        raw_input ("Press Ctrl+C or Enter to quit.");
    except (KeyboardInterrupt, SystemExit):
        pass;

    # Restore the backup
    shutil.copy(clientXmlPath + ".backup", clientXmlPath);
    print "\n ==== Everything has been restored ! :) ==== \n";
    sys.exit(0);