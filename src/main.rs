use tokio::net::TcpListener;
use tokio::sync::broadcast;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // setup graceful shutdown
    let (tx, mut rx) = broadcast::channel::<()>(1);

    tokio::spawn(async move {
        match tokio::signal::ctrl_c().await {
            Ok(_) => match tx.send(()) {
                Ok(_) => {},
                Err(_) => {
                    // TODO: log error
                }
            },
            Err(_) => {
                // TODO: log error
            }
        };
    });

    // start server
    let listener = TcpListener::bind("127.0.0.1:9922").await?;

    // accept new connection
    loop {
        let r = tokio::select! {
            r = listener.accept() => match r {
                Ok((c, a)) => Some((c, a)),
                Err(_) => {
                    // TODO: log error
                    None
                }
            },
            _ = rx.recv() => break
        };

        if let Some(_) = r {
            // TODO: handle connection
        }
    }

    Ok(())
}
