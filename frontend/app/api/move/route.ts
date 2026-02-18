import { NextResponse } from "next/server";
import { execFile } from "child_process";
import path from "path";

export async function POST(request: Request) {
  try {
    // Parse the incoming body
    const body = await request.json();
    const { fen } = body;

    // Validate input
    if (!fen) {
      return NextResponse.json({ error: "FEN string is required" }, { status: 400 });
    }

    // Locate the executable
    const enginePath = path.join(process.cwd(), "..", "engine", "chess_engine");

    // Execute the C Engine securely
    return new Promise((resolve) => {
      execFile(enginePath, [fen], (error, stdout, stderr) => {
        if (error) {
          console.error("Engine Error:", stderr || error.message);
          resolve(NextResponse.json({ error: "Engine failed to calculate move" }, { status: 500 }));
          return;
        }

        const bestMove = stdout.trim();
        resolve(NextResponse.json({ move: bestMove }));
      });
    });

  } catch (error) {
    return NextResponse.json({ error: "Internal Server Error" }, { status: 500 });
  }
}