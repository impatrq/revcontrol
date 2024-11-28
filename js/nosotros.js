document.addEventListener("DOMContentLoaded", () => {
    const mobileMenu = document.querySelector('.mobile-menu');
    const nav = document.querySelector('header nav');

    mobileMenu.addEventListener('click', () => {
        nav.classList.toggle('active');
    });

    const navLinks = document.querySelectorAll('nav a');
    navLinks.forEach(link => {
        link.addEventListener('click', (event) => {
            event.preventDefault();
            const targetPage = link.getAttribute('href');

            if (targetPage) {
                window.location.href = targetPage;
            }
        });
    });
});
