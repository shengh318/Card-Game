# Overview
[demonstration video](https://youtu.be/BsMHQ_h1Q08)
![State Machine](game_state_machine.pdf)

So our design project this time is to make a baccarat game machine using the deckofcards api to keep track of our cards and deck. 

Before starting to read this README, please have the state machine pdf open, it should be the link right under demonstration video.

There are multiple states that this game uses. 

The first state that I made is called bet. This is the screen that the player first sees when entering the game and it will also be the screen the player sees after each round. 
------------------------------------------------------------------------
On the betting screen the player will see:

player bets: #the amount of money that the player is betting on the player

banker bets: #the amount of money that the player is betting on the banker

player money: this is the total amount of money that the player currently has after each round of game. The player starts with a total of $100. Each bet takes 10 dollars; they can bet on both the player or the banker or both; the pay out for winning a bet is twice of what they put in. 

After the player clicks the player bet button or the banker bet button however many times their total amount of money allows them to, then they can click button 3 in order to get the game started. 

The game screen will look something like this:

Player Cards:
Banker Cards:
Player Total:
Banker Total:

Whoever won that round
------------------------------------------------------------------------
The game already has all of the baccarat rules programmed into it so the player can just see if they win or lose after they click button 3. 
------------------------------------------------------------------------
There are quite a few states to determin who wins. 

First there are the states called player auto win and banker auto win.

These two states are checked first since if either the player or the banker gets an 8 or a 9 on their first two cards, they win. Of course, if they both get an 8 or 9 on both of their cards, then its a tie and no one wins. The player does not win or lose money in the case of a tie. 

If the player or the banker did not get an 8 or a 9 on their 2 cards, then we follow the baccarat rules and draws cards automatically for the banker or the player. 
------------------------------------------------------------------------
After drawing, we see who wins and distribute the money accordingly. 

As you should be able to see, on the state machine diagram, all of the states goes back to the betting screen.
------------------------------------------------------------------------
Of course, if the player ever reaches the point of having 0 money, then the game gets reset. Every stat gets reset. The only thing that doesn't get reset is the deck if the deck still has enough cards left for games since in a casino, the dealer wouldn't switch decks just because you lost all your money.

However, if they never run out of money (and it's very hard to since each pay out is twice of what you put in), the game can go on forever.
------------------------------------------------------------------------
One big thing that I did that really organized everything for me was I write seperate helper functions for printing out the cards on the screen. If I were to just write

tft.println(player_1_cards);

this would have had to be written I think at least 20 something times for one iteration of the loop.

But, I was able to put the print statements inside of helper functions. Some examples you can see in the code that is called 

update_player_game_screen();
update_banker_game_screen();
update_both_game_screen();

Each of these functions are specifically made to help with the print statements. And since sometimes, only the player needs to draw cards, and vice versa; this is the reason why I decided to write different print functions rather than group them all into the same function. 

Also it helps to keep things organized. 
------------------------------------------------------------------------
