"use client";

import { useState, useEffect, useRef } from "react";
import { Chess, Square } from "chess.js";
import { Chessboard, defaultPieces } from "react-chessboard";

export default function ChessGame() {
  const [game, setGame] = useState(new Chess());
  const [moveFrom, setMoveFrom] = useState<Square | null>(null);
  const [optionSquares, setOptionSquares] = useState({});
  const [isCvC, setIsCvC] = useState(false);
  const [playerColor, setPlayerColor] = useState<"w" | "b">("w");
  const [boardFlipped, setBoardFlipped] = useState(false);
  const [pendingPromotion, setPendingPromotion] = useState<{ from: Square; to: Square } | null>(null);
  const moveListRef = useRef<HTMLDivElement>(null);

  // Auto-scroll move history to bottom
  useEffect(() => {
    if (moveListRef.current) {
      moveListRef.current.scrollTop = moveListRef.current.scrollHeight;
    }
  }, [game]);

  useEffect(() => {
    // Computer's turn: fires when it's not the player's color to move
    if (isCvC && game.turn() !== playerColor && !game.isGameOver()) {
      const timeout = setTimeout(async () => {
        try {
          const response = await fetch("/api/move", {
            method: "POST",
            body: JSON.stringify({ fen: game.fen() }),
          });

          const data = await response.json();

          if (data.move) {
            const from = data.move.slice(0, 2);
            const to = data.move.slice(2, 4);
            const promotion = data.move.length > 4 ? data.move[4] : "q";

            makeMove({ from, to, promotion });
          }
        } catch (error) {
          console.error("Error fetching move from engine:", error);
        }
      }, 500);

      return () => clearTimeout(timeout);
    }
  }, [game, isCvC, playerColor]);

  // --- Game Actions ---

  function isPromotion(from: Square, to: Square): boolean {
    const piece = game.get(from);
    if (!piece || piece.type !== "p") return false;
    return to[1] === "8" || to[1] === "1";
  }

  function makeMove(move: any) {
    const gameCopy = new Chess();
    gameCopy.loadPgn(game.pgn());

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
    if (isCvC && game.turn() !== playerColor) return;

    const squareKey = square as Square;

    if (moveFrom) {
      if (isPromotion(moveFrom, squareKey)) {
        const legalTargets = game.moves({ square: moveFrom, verbose: true }).map(m => m.to);
        if (legalTargets.includes(squareKey)) {
          setPendingPromotion({ from: moveFrom, to: squareKey });
          setMoveFrom(null);
          setOptionSquares({});
          return;
        }
      }
      const success = makeMove({ from: moveFrom, to: squareKey, promotion: "q" });
      if (success) return;
    }

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
            ? "radial-gradient(circle, transparent 40%, rgba(255,0,0,0.5) 40%)"
            : "radial-gradient(circle, rgba(0,0,0,.5) 25%, transparent 25%)",
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

  function onDrop({ sourceSquare, targetSquare }: { sourceSquare: string; targetSquare: string | null }) {
    if (isCvC && game.turn() !== playerColor) return false;
    if (!targetSquare) return false;

    const from = sourceSquare as Square;
    const to = targetSquare as Square;

    if (isPromotion(from, to)) {
      const legalTargets = game.moves({ square: from, verbose: true }).map(m => m.to);
      if (legalTargets.includes(to)) {
        setPendingPromotion({ from, to });
        return false;
      }
    }

    return makeMove({ from, to, promotion: "q" });
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
    gameCopy.undo();

    // In vs-computer mode, undo the engine's move too so it's the player's turn again
    if (isCvC && gameCopy.turn() !== playerColor) {
      gameCopy.undo();
    }

    setGame(gameCopy);
    setMoveFrom(null);
    setOptionSquares({});
  }

  function selectColor(color: "w" | "b") {
    setPlayerColor(color);
    setBoardFlipped(color === "b");
    resetGame();
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
      <div className="relative w-full max-w-[600px] aspect-square shadow-2xl">
        <Chessboard
          options={{
            position: game.fen(),
            boardOrientation: boardFlipped ? "black" : "white",
            onPieceDrop: onDrop,
            onSquareClick: onSquareClick,
            squareStyles: optionSquares,
          }}
        />

        {/* Promotion picker overlay */}
        {pendingPromotion && (() => {
          const color = game.turn();
          const pieces = ["q", "r", "b", "n"] as const;
          return (
            <div className="absolute inset-0 flex items-center justify-center bg-black/60 rounded z-10">
              <div className="bg-neutral-800 rounded-xl p-5 shadow-2xl border border-neutral-600">
                <p className="text-white text-center text-sm font-semibold mb-4 tracking-wide uppercase">
                  Promote to
                </p>
                <div className="flex gap-3">
                  {pieces.map(p => {
                    const key = `${color}${p.toUpperCase()}` as keyof typeof defaultPieces;
                    const Piece = defaultPieces[key];
                    return (
                      <button
                        key={p}
                        onClick={() => {
                          makeMove({ from: pendingPromotion.from, to: pendingPromotion.to, promotion: p });
                          setPendingPromotion(null);
                        }}
                        className="w-16 h-16 bg-neutral-700 hover:bg-neutral-500 rounded-lg flex items-center justify-center transition-colors"
                      >
                        <div style={{ width: 48, height: 48 }}>
                          <Piece svgStyle={{ width: "100%", height: "100%" }} />
                        </div>
                      </button>
                    );
                  })}
                </div>
              </div>
            </div>
          );
        })()}
      </div>

      {/* Controls */}
      <div className="flex flex-col w-full md:w-80 h-[600px] bg-neutral-800 rounded-lg p-6 shadow-xl space-y-6">

        {/* Status Header */}
        <div className="text-center pb-4 border-b border-neutral-700">
          <h2 className={`text-2xl font-bold ${isGameOver ? "text-red-500" : "text-white"}`}>
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
                    <div className="text-neutral-500 text-right pr-3">{index / 2 + 1}.</div>
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

          {/* Color picker — only shown in vs-computer mode */}
          {isCvC && (
            <div className="flex gap-2">
              <button
                onClick={() => selectColor("w")}
                className={`flex-1 py-2 rounded font-semibold transition-colors border-2 ${
                  playerColor === "w"
                    ? "bg-white text-neutral-900 border-white"
                    : "bg-neutral-700 text-neutral-300 border-neutral-600 hover:bg-neutral-600"
                }`}
              >
                Play White
              </button>
              <button
                onClick={() => selectColor("b")}
                className={`flex-1 py-2 rounded font-semibold transition-colors border-2 ${
                  playerColor === "b"
                    ? "bg-neutral-900 text-white border-white"
                    : "bg-neutral-700 text-neutral-300 border-neutral-600 hover:bg-neutral-600"
                }`}
              >
                Play Black
              </button>
            </div>
          )}

          {/* Flip Board */}
          <button
            onClick={() => setBoardFlipped(f => !f)}
            className="px-4 py-2 bg-neutral-600 hover:bg-neutral-500 text-neutral-200 rounded font-semibold transition-colors"
          >
            Flip Board
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
