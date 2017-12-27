## Team C-rescendo - Anish Shenoy, Ashneel Das, Jasper Cheung

# Project Description
This project aims to allow users at a party or similar event with many people to vote for songs on a shared group playlist. The music that is voted highest will be put at the top of a playlist that is shared by all users. Users will also have access to their own music library (similar to the music library created in project 0), which other users cannot alter.

# How to use the project
- The music will be in the form of .WAV files. 
- One computer will be the main computer that hosts the shared playlist. This computer can also play the playlist by using a “-p” flag. This computer can also reset the playlist using a “-r” flag.
- Users will place their .wav files into a directory. The program will read in these files and ask the user for information such as the song name, artist, and genre.
- Users will have access to their private library and one shared playlist. 
- They will have the option to upvote or downvote a song in the main playlist by typing the name of the .WAV file followed by a flag (“-u” for upvote, “-d” for downvote).
- The user can play songs from their own library with the “-p” flag.

# Technical Design
#### Topics Covered:  

- Networking: Users will be able to access the shared playlist from different computers
Shared Memory: The shared playlist will be stored as shared memory
- Reading/Writing to files: A user’s library will be stored as a file. The program will also have to access the .wav files in a directory. The shared playlist will also be written to a file when the “server” (the computer that hosts the shared playlist) exits in order to preserve it in between uses.
- Linked Lists/Structs: The library will be organized using linked lists.
- User input: Read in commands from the terminal
- Semaphores: Only one user should be able to add to the shared playlist at a time. 
- External library: https://github.com/lichray/wavplay

#### Breakdown of work: 

- Ashneel: Local music player (individual playlists/playing music)
- Anish: Server aspect of the project (shared playlist)
- Jasper: Connection between the individual and shared playlist (voting, etc.) 
- All 3 of us!: Will work on figuring out how to use the wavplay library and how to use networking (and, of course, help each other out should we run into troubles)

# Timeline
- Dec 27th: submit our design document to our glorious computer science instructor
- By Jan 2: Gain an understanding of the WAVPlay library so that we can use it to play songs.
- By Jan 5: Hopefully gain an understanding of networking/sockets and how we can implement it into our program.
- By Jan 8: Implement basic functionality of our program for playing music and creating playlists. 
- By Jan 12: Have a functioning server through which users can vote and order songs by number of votes. 
- By Jan 15: Implement semaphores and shared memory so that all users will have access to the shared playlist and only one user can vote at a time. 
- By Jan 19: Add finishing touches and submit project!
