"use client";

import { useState, useEffect, useRef } from "react";
import { Chess, Square } from "chess.js";
import { Chessboard } from "react-chessboard";

export default function ChessGame() {
  const [game, setGame] = useState(new Chess());
  const [moveFrom, setMoveFrom] = useState<Square | null>(null);
  const [optionSquares, setOptionSquares] = useState({});
  const [isCvC, setIsCvC] = useState(false); // Toggle: Player vs Player (false) or Player vs CPU (true)
  const moveListRef = useRef<HTMLDivElement>(null);

  // Auto-scroll move history to bottom
  useEffect(() => {
    if (moveListRef.current) {
      moveListRef.current.scrollTop = moveListRef.current.scrollHeight;
    }
  }, [game]);

  useEffect(() => {
    // Computer's turn
    if (isCvC && game.turn() === "b" && !game.isGameOver()) {
      const timeout = setTimeout(async () => {
        try {
          const response = await fetch("/api/move", {
            method: "POST",
            body: JSON.stringify({ fen: game.fen() }),
          });

          const data = await response.json();

          if (data.move) {
            const from = data.move.slice(0, 2);
            const to = data.move.substring(2, 4);

            makeMove({ from, to, promotion: "q" });
          }
        } catch (error) {
          console.error("Error fetching move from engine:", error);
        }
      }, 500);
      
      return () => clearTimeout(timeout);
    }
  }, [game, isCvC]);

  // --- Game Actions ---

  function makeMove(move: any) {
    const gameCopy = new Chess();
    gameCopy.loadPgn(game.pgn()); // Load PGN to preserve history

    try {
      const result = gameCopy.move(move);
      if (result) {
        setGame(gameCopy);
        setMoveFrom(null);
        setOptionSquares({});
        return true;
      }
    } catch (e) {
      return false;
    }
    return false;
  }

  function onSquareClick({ square }: { square: string }) {
    // Prevent player from clicking if it's Computer's turn
    if (isCvC && game.turn() === "b") return;

    const squareKey = square as Square;

    // Attempt to move
    if (moveFrom) {
      const success = makeMove({
        from: moveFrom,
        to: squareKey,
        promotion: "q",
      });
      if (success) return;
    }

    // Select piece & highlight
    const moves = game.moves({ square: squareKey, verbose: true });
    if (moves.length > 0) {
      setMoveFrom(squareKey);
      const newSquares: Record<string, { background: string; borderRadius?: string }> = {};
      
      moves.map((move) => {
        const targetSquare = move.to as Square;
        const targetPiece = game.get(targetSquare);
        const sourcePiece = game.get(squareKey);
        const isCapture = targetPiece && sourcePiece && targetPiece.color !== sourcePiece.color;

        newSquares[move.to] = {
          background: isCapture
              ? "radial-gradient(circle, transparent 40%, rgba(255,0,0,0.5) 40%)" // Red ring
              : "radial-gradient(circle, rgba(0,0,0,.5) 25%, transparent 25%)",   // Grey dot
          borderRadius: "50%",
        };
        return move;
      });
      newSquares[squareKey] = { background: "rgba(255, 255, 0, 0.4)" };
      setOptionSquares(newSquares);
    } else {
      setMoveFrom(null);
      setOptionSquares({});
    }
  }

  function onDrop({ sourceSquare, targetSquare }: { sourceSquare: string, targetSquare: string | null }) {
    if (isCvC && game.turn() === "b") return false; // Prevent dragging during AI turn
    if (!targetSquare) return false;
    
    return makeMove({
      from: sourceSquare as Square,
      to: targetSquare as Square,
      promotion: "q",
    });
  }

  // --- Controls ---

  function resetGame() {
    setGame(new Chess());
    setMoveFrom(null);
    setOptionSquares({});
  }

  function undoMove() {
    const gameCopy = new Chess();
    gameCopy.loadPgn(game.pgn());
    gameCopy.undo(); // Undo last move
    
    // Undos an extra time if playing vs Computer
    if (isCvC && gameCopy.turn() === "b") {
        gameCopy.undo(); 
    }

    setGame(gameCopy);
    setMoveFrom(null);
    setOptionSquares({});
  }

  // --- Status Logic ---

  const isGameOver = game.isGameOver();
  let status = "";
  if (game.isCheckmate()) status = `Checkmate! ${game.turn() === "w" ? "Black" : "White"} wins.`;
  else if (game.isDraw()) status = "Draw!";
  else if (game.isGameOver()) status = "Game Over!";
  else status = `${game.turn() === "w" ? "White" : "Black"}'s turn`;

  return (
    <div className="flex flex-col md:flex-row items-center justify-center h-screen w-full bg-neutral-900 p-8 gap-8 font-sans">
      
      {/* Chessboard */}
      <div className="w-full max-w-[600px] aspect-square shadow-2xl">
        <Chessboard
          options={{
            position: game.fen(),
            boardOrientation: "white",
            onPieceDrop: onDrop,
            onSquareClick: onSquareClick,
            squareStyles: optionSquares,
          }}
        />
      </div>

      {/* Controls */}
      <div className="flex flex-col w-full md:w-80 h-[600px] bg-neutral-800 rounded-lg p-6 shadow-xl space-y-6">
        
        {/* Status Header */}   
        <div className="text-center pb-4 border-b border-neutral-700">
          <h2 className={`text-2xl font-bold ${isGameOver ? 'text-red-500' : 'text-white'}`}>
            {status}
          </h2>
          {game.isCheck() && !isGameOver && (
             <p className="text-red-400 font-semibold mt-2 animate-pulse">Check!</p>
          )}
        </div>

        {/* Move History */}
        <div className="flex-1 overflow-y-auto bg-neutral-900/50 rounded p-4 font-mono text-sm" ref={moveListRef}>
          <div className="grid grid-cols-[3rem_1fr_1fr] gap-y-1 items-center">
            {game.history().map((move, index) => {
              if (index % 2 === 0) {
                return (
                  <div key={index} className="contents">
                    <div className="text-neutral-500 text-right pr-3">{(index / 2) + 1}.</div>
                    <div className="text-white font-medium">{move}</div>
                  </div>
                );
              } else {
                return <div key={index} className="text-white font-medium">{move}</div>;
              }
            })}
          </div>
        </div>

        {/* Buttons */}
        <div className="flex flex-col gap-3 pt-2">
          {/* Mode Toggle */}
          <button 
            onClick={() => { setIsCvC(!isCvC); resetGame(); }}
            className={`px-4 py-2 rounded font-semibold transition-colors ${
              isCvC ? "bg-blue-600 hover:bg-blue-700 text-white" : "bg-neutral-600 hover:bg-neutral-500 text-neutral-200"
            }`}
          >
            {isCvC ? "Mode: Vs Computer" : "Mode: 2 Player"}
          </button>

          <div className="grid grid-cols-2 gap-4">
            <button 
              onClick={undoMove}
              disabled={game.history().length === 0 || isGameOver}
              className="px-4 py-2 bg-neutral-700 hover:bg-neutral-600 disabled:opacity-50 text-white rounded font-semibold transition-colors"
            >
              Undo
            </button>
            <button 
              onClick={resetGame}
              className="px-4 py-2 bg-red-600 hover:bg-red-700 text-white rounded font-semibold transition-colors"
            >
              Reset
            </button>
          </div>
        </div>

      </div>
    </div>
  );
}