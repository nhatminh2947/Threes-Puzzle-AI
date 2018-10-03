# Threes-Demo
<br>
<p>
Computer Games and Intelligence (CGI) Lab, NCTU, Taiwan<br>
http://www.aigames.nctu.edu.tw/<br>
<p>

<p>Board size is 4x4.<br>
When the game starts, every cell either is empty or has one 𝑣-tile, where 𝑣 is 1, 2 or 3. <br>
The sliding distance is at most one.<br>
In the rules of merging, a 1-tile and a 2-tile can be merged into a 3-tile, and two 𝑣-tiles can be merged into a 
2𝑣-tile like the game 2048, where 2 < 𝑣 < 6144. Note that 6144-tiles cannot be merged anymore.
Initially, there is a bag of 3 tiles 1, 2, and 3-tiles. 

A Threes game ends similarly when the player is no longer able to make any legal move.
The final score is the sum of scores of all 𝑣-tiles with 𝑣 ≥ 3, using the formula: 

![equation](http://latex.codecogs.com/gif.latex?3%5E%7Blog_2%28%5Cfrac%7Bv%7D%7B3%7D%29%20&plus;%201%7D)

The objective of this game is to achieve as much score as possible. 
<p>