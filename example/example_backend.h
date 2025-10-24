// A simple interface for a backend.
class ExampleBackend {
public:
    virtual ~ExampleBackend() = default;
    virtual void run_example(Theme theme, RenderAPI renderApi) = 0;
};