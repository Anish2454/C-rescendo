# Team C-rescendo 
## Anish Shenoy, Ashneel Das, Jasper Cheung
### Period 5

# Project Description
Our project is a server and client based shared playlist. This project aims to allow users at a party or similar event with many people to vote for songs on a shared group playlist. The music that is voted highest will be put at the top of a playlist that is shared by all users. Users will also have access to their own music library (similar to the music library created in project 0), which other users cannot alter.

# Required Libraries 
### MPG123
```
$ sudo apt-get install mpg123
```

# Instructions
1. Clone Repo 
2. Run `$ make`
## Client-Side
1. First run the client(local) playlist with `$ ./client`
2. To add songs to your playlist enter `add` when the client prompts for an input
3. Type the name, artist, and filename(make sure file is in the main directory)
4. Repeat for as many times as you want
5. `remove` to remove songs from your playlist
6. `play` to play your playlist
7. `stop` to stop playing a song
8. `view` to view client(local) playlist
## Server-Side
1. In another computer open the server with `$ ./server`
2. Type the how long the waits server to start playing the first song
3. Server will listen for connections and votes
## Connected
1. Once you run server, you can connect to the server with your clients
by using `server` and giving the ip of the server. You can then
specify the amount of time to wait for initial votes. 
2. On the client you can vote for songs with `vote -<song name> -<artist>`
3. You can also view the shared playlist on the client with `view`
4. The Server will take in the votes and play what has the most votes
5. You can vote for the next song while a song plays

ENJOY THE BOPS
