import java.io.*;
import java.net.*;

class tcpserver{
    public static void main(String args[]) throws Exception{
        ServerSocket listenSocket = new ServerSocket(9876);
        while(true){
            Socket connectionSocket = listenSocket.accept();
            BufferedReader inFromClient =
                new BufferedReader(new InputStreamReader(connectionSocket.getInputStream()));
            DataOutputStream outToClient = 
                new DataOutputStream(connectionSocket.getOutputStream());
            String clientMessage = inFromClient.readLine();
            System.out.println("The client said: " + clientMessage);
            connectionSocket.close();
        }

    }
}
