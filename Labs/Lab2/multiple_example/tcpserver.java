import java.io.*;
import java.net.*;

class tcpserver{
    public static void main(String args[]) throws Exception{
        ServerSocket listenSocket = new ServerSocket(9876);
        while(true){
            Socket s = listenSocket.accept();
            Runnable r = new clientHandler(s);
            Thread t = new Thread(r);
            //Will need to use synchronized eventually
            t.start();

        }
    }
}


class clientHandler implements Runnable{
    socket connectionSocket;
    clientHandler(Socket connection){

        connectionSocket = connection;
    }

    public void run(){
        try{
            BufferedReader inFromClient =
                new BufferedReader(new InputStreamReader(connectionSocket.getInputStream()));
            DataOutputStream outToClient = 
                new DataOutputStream(connectionSocket.getOutputStream());
            String clientMessage = inFromClient.readLine();
            System.out.println("The client said: " + clientMessage);
            connectionSocket.close();

        }catch (Exception e){

            System.out.println("Got an exception");
        }

    }
}
