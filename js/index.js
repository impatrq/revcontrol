document.addEventListener("DOMContentLoaded", () => {
    const mobileMenuIcon = document.querySelector('.mobile-menu');
    const navMenu = document.querySelector('header nav');

    mobileMenuIcon.addEventListener('click', () => {
        navMenu.classList.toggle('active'); 
    });

    const navLinks = document.querySelectorAll('header nav a');
    navLinks.forEach(link => {
        link.addEventListener('click', (event) => {
            event.preventDefault(); 

            const targetUrl = link.getAttribute('href'); 
            if (targetUrl === "index.html") {
                console.log("Ya estás en la página de inicio");
            } else {
                window.location.href = targetUrl;
            }
        });
    });
});
